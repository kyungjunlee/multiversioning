#include "batch/lock_queue.h"
#include "util.h"

#include <cassert>

void BatchLockQueue::non_concurrent_push_tail(LockStage&& ls) {
  non_concurrent_push_tail_implem(
      new typename MSQueue<LockStage>::QueueElt(
        std::forward<LockStage>(ls)));
}

void BatchLockQueue::non_concurrent_push_tail(const LockStage& ls) {
  non_concurrent_push_tail_implem(
      new typename MSQueue<LockStage>::QueueElt(ls));
}

void BatchLockQueue::non_concurrent_push_tail_implem(
    typename MSQueue<LockStage>::QueueElt* ls) {
  this->tail->set_next_elt(ls);
  this->tail = this->tail->get_next_elt(); 
}
