#ifndef _LOCK_STAGE_H_
#define _LOCK_STAGE_H_

#include "batch/lock_types.h"
#include "batch/batch_action_interface.h"

#include <memory>
#include <stdint.h>
#include <unordered_set>

// TODO: Override the new/delete operators
// TODO: implement a public inheritance test class which provides a == operator.
/*
 *  LockStage
 *    Logically, lockstage corresponds to a set of transactions that can run concurrently
 *    on a given record with no inconsistencies resulting. Hence, if an exclusive lock
 *    is requested, only one transaction may be within a lock stage. However, with a 
 *    shared lock being requested, any number of transactions may be a part of a lock stage.
 *
 *    A lock stage is considered nearly immutable after it has been handed off by a scheduling thread.
 *    The only field that may be updated is the holders integer, which is handled using FAI/FAD
 *    instructions. 
 *
 *    Lock stages form a singly linked list. Moving towards the "next_stage" means moving deeper into
 *    the dependency graph. This would likely be done to pass on a lock. 
 */
class LockStage {
public:
  typedef std::unordered_set<std::shared_ptr<IBatchAction>> RequestingActions;

protected:
  // The number of transactions holding on to the lock.
  uint64_t holders;
  LockType l_type;
  RequestingActions requesters;

  // this is just to assert that no stage is given a lock more than once.
  uint64_t has_been_given_lock;

public:
  LockStage();
  LockStage(
      RequestingActions requesters,
      LockType lt);

  // Returns true if successful and false otherwise
  //
  // May only be called in a single-threaded scenarios. We do not coalesce adjacent shared stages
  // into single stages.
  bool add_to_stage(std::shared_ptr<IBatchAction> txn, LockType lt);
  // Returns the new value of holders
  uint64_t decrement_holders(); 
  
  const RequestingActions& get_requesters() const; 
  uint64_t get_holders() const;

  // return true if all actions within this lock stage have been finished.
  bool finalize_action(std::shared_ptr<IBatchAction> act);
  void notify_lock_obtained();
  bool has_lock();

  friend bool operator==(const LockStage& ls1, const LockStage& ls2);
};

#endif // _LOCK_STAGE_H_
