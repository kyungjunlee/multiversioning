#include "batch/lock_queue.h"
#include "util.h"

#include <cassert>

void BatchLockQueue::non_concurrent_push_tail(
    std::shared_ptr<LockStage>&& ls) {
  non_concurrent_push_tail_implem(
      std::make_shared
        <typename SPSCMRQueue<std::shared_ptr<LockStage>>::QueueElt>
       (std::move(ls)));
}

void BatchLockQueue::non_concurrent_push_tail(
    const std::shared_ptr<LockStage>& ls) {
  non_concurrent_push_tail_implem(
      std::make_shared
        <typename SPSCMRQueue<std::shared_ptr<LockStage>>::QueueElt>(ls));
}

void BatchLockQueue::non_concurrent_push_tail_implem(
    std::shared_ptr<typename SPSCMRQueue<std::shared_ptr<LockStage>>::QueueElt> ls) {
  if (tail == nullptr) {
    assert(head == nullptr);
    head = ls;
    tail = ls;
  } else {
    tail->set_next_elt(ls);
    tail = ls; 
  }
}
