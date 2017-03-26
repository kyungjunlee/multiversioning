#include "batch/MS_queue.h"
#include "util.h"

#include <cassert>
#include <utility>

// Definitions of QueueElt functions.
template <typename Elt>
MSQueue<Elt>::QueueElt::QueueElt() :
  next(nullptr)
{};

template <typename Elt>
MSQueue<Elt>::QueueElt::QueueElt(const Elt& e):
  contents(e),
  next(nullptr)
{};

template <typename Elt>
MSQueue<Elt>::QueueElt::QueueElt(Elt&& e):
  contents(std::move(e)),
  next(nullptr)
{};

template <typename Elt>
Elt& MSQueue<Elt>::QueueElt::get_contents() {
  return contents;
};

template <typename Elt>
typename MSQueue<Elt>::QueueElt* MSQueue<Elt>::QueueElt::get_next_elt() {
  return next;
};

template <typename Elt>
bool MSQueue<Elt>::QueueElt::set_next_elt(MSQueue<Elt>::QueueElt* ne) {
  return cmp_and_swap((uint64_t*) &next, 0, (uint64_t) ne); 
};

template <typename Elt>
// Definitions of the MSQueue functions.
MSQueue<Elt>::MSQueue() {
  QueueElt* dummy_node = new QueueElt();
  head = dummy_node;
  tail = dummy_node;
}

template <typename Elt>
bool MSQueue<Elt>::is_empty() const {
  barrier();
  return (head == tail);
}

template <typename Elt>
typename MSQueue<Elt>::QueueElt* MSQueue<Elt>::peek_head_elt() const {
  return head->get_next_elt();
}

template <typename Elt>
Elt& MSQueue<Elt>::peek_head() const {
  QueueElt* actual_head = peek_head_elt();
  assert(actual_head != nullptr);

  return actual_head->get_contents();
}

template <typename Elt>
typename MSQueue<Elt>::QueueElt* MSQueue<Elt>::peek_tail_elt() const {
  if (is_empty()) return nullptr;

  return tail;
}

template <typename Elt>
Elt& MSQueue<Elt>::peek_tail() const {
  QueueElt* tail = peek_tail_elt();
  assert(tail != nullptr);

  return tail->get_contents();
}

template <typename Elt>
void MSQueue<Elt>::pop_head() {
  QueueElt* m_head;
  QueueElt* m_tail;
  QueueElt* m_next;
  while(true) {
    m_head = head;
    m_tail = tail;
    m_next = m_head->get_next_elt();
    barrier();
    if (head == m_head) {               // head didn't change
      if (m_head == m_tail) {           // empty or tail behind
        if (m_next == nullptr) {
          assert(false);               // empty
        } 
        
        continue;
      }
      assert(m_next != nullptr);

      // not empty and the tail is not lagging behind
      if (cmp_and_swap(
            (uint64_t*) &head,
            (uint64_t) m_head,
            (uint64_t) m_next)) break;
    }
  }  

  delete m_head;
}

template <typename Elt>
void MSQueue<Elt>::push_tail(Elt&& e) {
  push_tail_implem(new QueueElt(std::move(e)));  
}

template <typename Elt>
void MSQueue<Elt>::push_tail(const Elt& e) {
  push_tail_implem(new QueueElt(e));
};

template <typename Elt>
void MSQueue<Elt>::push_tail_implem(QueueElt* qe) {
  assert(qe != nullptr);
  QueueElt* m_tail = tail;
  barrier();

  // single consumer, single producer means that this must
  // succeed on the first attempt
  bool CAS_success = false;
  assert(m_tail->get_next_elt() == nullptr);
  // set the next stage
  CAS_success = m_tail->set_next_elt(qe);
  assert(CAS_success);

  // swing the tail around.
  CAS_success= cmp_and_swap(
     (uint64_t*) &tail, 
     (uint64_t) m_tail,
     (uint64_t) qe);
  assert(CAS_success); 
};

template <typename Elt>
void MSQueue<Elt>::merge_queue(MSQueue<Elt>* lq) {
  if (lq->is_empty()) return;

  QueueElt* m_tail = tail;
  QueueElt* lq_head = lq->peek_head_elt();
  QueueElt* lq_tail = lq->peek_tail_elt();
  barrier();

  // single produced and single consumer model in this modified queue
  // means that only merge may move tail and so it is free to only 
  // CAS once.
  bool CAS_success = false;
  assert(lq_tail->get_next_elt() == nullptr);
  assert(m_tail->get_next_elt() == nullptr);
  // set the next stage
  CAS_success = m_tail->set_next_elt(lq_head);
  assert(CAS_success);

  // swing the tail around.
  CAS_success= cmp_and_swap(
     (uint64_t*) &tail, 
     (uint64_t) m_tail,
     (uint64_t) lq_tail);
  assert(CAS_success); 

  // remove all the elements from the lq so that
  // destructor may handle it.
  lq->tail = lq->head;
}

template <typename Elt>
MSQueue<Elt>::~MSQueue() {
  while (!is_empty()) {
    pop_head();
  }

  delete head;
};
