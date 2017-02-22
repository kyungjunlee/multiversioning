#include "batch/lock_queue.h"
#include "util.h"

void BatchLockQueue::non_concurrent_push_tail(LockStage* ls) {
  this->tail->set_next_elt(new typename MSQueue<LockStage>::QueueElt(ls));
  this->tail = this->tail->get_next_elt();
}
