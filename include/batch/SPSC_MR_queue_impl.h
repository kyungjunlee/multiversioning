#ifndef SPSC_MR_QUEUE_IMPL_H_
#define SPSC_MR_QUEUE_IMPL_H_

#include "batch/SPSC_MR_queue.h"
#include "batch/mutex_rw_guard.h"

#include <cassert>

template <typename Elt>
SPSCMRQueue<Elt>::QueueElt::QueueElt():
  next(nullptr)
{};

template <typename Elt>
SPSCMRQueue<Elt>::QueueElt::QueueElt(const Elt& e) :
  QueueElt()
{
  contents = std::make_shared<Elt>(e);
};

template <typename Elt>
SPSCMRQueue<Elt>::QueueElt::QueueElt(Elt&& e):
  QueueElt()
{
  contents = std::make_shared<Elt>(std::move(e));
};

template <typename Elt>
std::shared_ptr<Elt> SPSCMRQueue<Elt>::QueueElt::get_contents() {
  return contents;
};

template <typename Elt>
std::shared_ptr<typename SPSCMRQueue<Elt>::QueueElt> 
SPSCMRQueue<Elt>::QueueElt::get_next_elt() {
  return next;
};

template <typename Elt>
void SPSCMRQueue<Elt>::QueueElt::set_next_elt(
    std::shared_ptr<typename SPSCMRQueue<Elt>::QueueElt> ne) {
  next = ne;
}

template <typename Elt>
SPSCMRQueue<Elt>::SPSCMRQueue() {
  pthread_rwlock_init(&lock, NULL);
  tail = nullptr;
  head = nullptr;
};

template <typename Elt> 
bool SPSCMRQueue<Elt>::is_empty() const {
   MutexRWGuard g((pthread_rwlock_t*) &lock, LockType::shared);  
  return head == nullptr && tail == nullptr;
};

template <typename Elt>
std::shared_ptr<typename SPSCMRQueue<Elt>::QueueElt> 
SPSCMRQueue<Elt>::peek_tail_elt() const {
  return tail; 
}

template <typename Elt>
std::shared_ptr<typename SPSCMRQueue<Elt>::QueueElt> 
SPSCMRQueue<Elt>::peek_head_elt() const {
  return head;
}

template <typename Elt>
std::shared_ptr<Elt> SPSCMRQueue<Elt>::peek_tail() const {
   MutexRWGuard g((pthread_rwlock_t*) &lock, LockType::shared);
  return is_empty() ? nullptr : peek_tail_elt()->get_contents(); 
}

template <typename Elt>
std::shared_ptr<Elt> SPSCMRQueue<Elt>::peek_head() const {
   MutexRWGuard g((pthread_rwlock_t*) &lock, LockType::shared);
  return is_empty() ? nullptr : peek_head_elt()->get_contents();
}

template <typename Elt>
void SPSCMRQueue<Elt>::pop_head() {
  if (is_empty()) {
    return;
  } 
  
  MutexRWGuard g((pthread_rwlock_t*) &lock, LockType::exclusive);
  auto next = head->get_next_elt();
  if (next == nullptr) {
    assert(head == tail);
    head = nullptr;
    tail = nullptr;
  }  else {
    head = next;
  }
}

template <typename Elt>
void SPSCMRQueue<Elt>::push_tail(const Elt& e) {
  push_tail_implem(std::make_shared<QueueElt>(e));
}

template <typename Elt>
void SPSCMRQueue<Elt>::push_tail(Elt&& e) {
  push_tail_implem(std::make_shared<QueueElt>(std::move(e)));
}

template <typename Elt>
void SPSCMRQueue<Elt>::push_tail_implem(
    std::shared_ptr<typename SPSCMRQueue<Elt>::QueueElt> qe) {
   MutexRWGuard g((pthread_rwlock_t*) &lock, LockType::exclusive);

  if (tail == nullptr) {
    tail = qe;
    head = tail;
  } else {
    tail->set_next_elt(qe);
    tail = qe;  
  }  
}

template <typename Elt>
void SPSCMRQueue<Elt>::merge_queue(SPSCMRQueue<Elt>* lq) {
   if (lq->is_empty()) return;

   MutexRWGuard g((pthread_rwlock_t*) &lock, LockType::exclusive);
   if (head == nullptr) {
    assert(tail == nullptr);
    tail = lq->peek_tail_elt();
    head = lq->peek_head_elt();
   } else {
    assert(tail->get_next_elt() == nullptr);
    assert(lq->tail->get_next_elt() == nullptr);
    tail->set_next_elt(lq->peek_head_elt());
    tail = lq->peek_tail_elt();
   }
}

template <typename Elt>
SPSCMRQueue<Elt>::~SPSCMRQueue() {
  pthread_rwlock_destroy(&lock);
}

#endif // SPSC_MR_QUEUE_IMPL_H_
