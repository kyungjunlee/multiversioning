#ifndef SPSC_MR_QUEUE_H_
#define SPSC_MR_QUEUE_H_

#include <memory>
#include <pthread.h>

// Single Producer, Single consumer, Multiple Reader Queue
//
// Only one thread should ever push (or merge) at a time and one thread
// should ever pop at a time, however many threads may do this over time.
// Moreover, many threads may read at a time. The element returned might
// become "outdated", but will not be deallocated until the last of 
// the users has finished processing the elements. 
//
// Please note the "safe" and "unsafe" fields and methods within QueueElt.
// All methods within SPSCMRQueue are thread safe.
template <typename Elt>
class SPSCMRQueue {
public:
  SPSCMRQueue();
  pthread_rwlock_t lock;

  class QueueElt {
    public:
      QueueElt();
      QueueElt(const Elt& e);
      QueueElt(Elt&& e);
      // contents pointer is considered immutable after the elements
      // appears in the queue. That is, the elt may change, but the
      // pointer does not.
      std::shared_ptr<Elt> contents;
      // next is not safe to peak at from outside of SPSCMRQueue.
      std::shared_ptr<QueueElt> next;

      std::shared_ptr<Elt> get_contents();
      // these functions are NOT thread safe.
      std::shared_ptr<QueueElt> get_next_elt();
      void set_next_elt(std::shared_ptr<QueueElt> ne);
  };

  std::shared_ptr<QueueElt> head;
  std::shared_ptr<QueueElt> tail;

  bool is_empty() const;
  
  std::shared_ptr<QueueElt> peek_tail_elt() const;
  std::shared_ptr<QueueElt> peek_head_elt() const;
  std::shared_ptr<Elt> peek_tail() const;
  std::shared_ptr<Elt> peek_head() const;

  void pop_head();
  void push_tail(const Elt& e);
  void push_tail(Elt&& e);

  void merge_queue(SPSCMRQueue<Elt>* lq);

  ~SPSCMRQueue();

private:
  void push_tail_implem(std::shared_ptr<QueueElt> qe);  
};

#include "batch/SPSC_MR_queue_impl.h"

#endif // SPSC_MR_QUEUE_H_

