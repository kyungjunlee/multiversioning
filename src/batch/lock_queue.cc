#include "batch/lock_queue.h"
#include "util.h"

#include <cassert>

void BatchLockQueue::non_concurrent_push_tail(
    std::shared_ptr<LockStage>&& ls) {
  non_concurrent_push_tail_implem(
      new typename MSQueue<std::shared_ptr<LockStage>>::QueueElt(
        std::move(ls)));
}

void BatchLockQueue::non_concurrent_push_tail(
    const std::shared_ptr<LockStage>& ls) {
  non_concurrent_push_tail_implem(
      new typename MSQueue<std::shared_ptr<LockStage>>::QueueElt(ls));
}

void BatchLockQueue::non_concurrent_push_tail_implem(
    typename MSQueue<std::shared_ptr<LockStage>>::QueueElt* ls) {
  this->tail->set_next_elt(ls);
  this->tail = this->tail->get_next_elt(); 
}
