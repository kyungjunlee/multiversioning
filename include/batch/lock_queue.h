#ifndef _LOCK_QUEUE_H_
#define _LOCK_QUEUE_H_

#include "batch/lock_stage.h"
#include "batch/MS_queue.h"

#include <memory>

class BatchLockQueue;

// TODO:
//    - Memory Management. Right now we have memory leakage within the destructor
//    since the lock stages are never deallocated!
//    - Overloading of new/delete.

/*
 * LockQueue
 *    
 *    A queue of lock stages implemented as a MS-queue. See batch/MS_queue.h for details. 
 */
class LockQueue : public MSQueue<std::shared_ptr<LockStage>> {
private:
  using MSQueue<std::shared_ptr<LockStage>>::push_tail;
};

/*
 * BatchLockQueue
 *
 *    Basically a LockQueue with the added functionality of non-thread safe
 *    addition of lock stages. Used only by the scheduling threads before
 *    merging into the global schedule.
 */
class BatchLockQueue : public MSQueue<std::shared_ptr<LockStage>> {
private:
  void non_concurrent_push_tail_implem(
      typename MSQueue<std::shared_ptr<LockStage>>::QueueElt* ls);
public:
  // TODO:
  //    This creates an element using the new directive. Make sure to override
  //    it and return memory whenever necessary.
  void non_concurrent_push_tail(std::shared_ptr<LockStage>&& ls);
  void non_concurrent_push_tail(const std::shared_ptr<LockStage>& ls);
};

#endif // _LOCK_QUEUE_H_
