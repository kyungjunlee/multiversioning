#ifndef _LOCK_QUEUE_H_
#define _LOCK_QUEUE_H_

#include "batch/lock_stage.h"
#include "batch/MS_queue.h"

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
class LockQueue : public MSQueue<LockStage> {
private:
  using MSQueue<LockStage>::push_tail;
};

/*
 * BatchLockQueue
 *
 *    Basically a LockQueue with the added functionality of non-thread safe
 *    addition of lock stages. Used only by the scheduling threads before
 *    merging into the global schedule.
 */
class BatchLockQueue : public MSQueue<LockStage> {
private:
  void non_concurrent_push_tail_implem(typename MSQueue<LockStage>::QueueElt* ls);
public:
  // TODO:
  //    This creates an element using the new directive. Make sure to override
  //    it and return memory whenever necessary.
  void non_concurrent_push_tail(LockStage&& ls);
  void non_concurrent_push_tail(const LockStage& ls);
};

#endif // _LOCK_QUEUE_H_
