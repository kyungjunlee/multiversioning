#include "batch/lock_stage.h"
#include "util.h"

#include <cassert>

LockStage::LockStage(): 
    holders(0),
    l_type(LockType::shared),
    requesters({}),
    has_been_given_lock(false)
  {};

LockStage::LockStage(
      RequestingActions requesters,
      LockType lt):
    holders(requesters.size()),
    l_type(lt),
    requesters(requesters),
    has_been_given_lock(false)
  {
    assert(!(lt == LockType::exclusive && requesters.size() > 1));
  };

bool LockStage::add_to_stage(std::shared_ptr<IBatchAction> txn, LockType lt) {
  // can only add to a stage when both the request and the stage are shared
  // or when the stage is empty.
  if ((lt == LockType::exclusive && requesters.size() > 0) ||
      l_type == LockType::exclusive) {
    return false;
  }

  requesters.insert(txn);
  l_type = lt;
  holders ++;
  return true;
};

uint64_t LockStage::decrement_holders() {
  return fetch_and_decrement(&holders);
};

const LockStage::RequestingActions& LockStage::get_requesters() const {
  return requesters;
};

uint64_t LockStage::get_holders() const {
  return holders;
};

bool LockStage::finalize_action(std::shared_ptr<IBatchAction> act) {
  // act is a part of this stage!
  assert(requesters.find(act) != requesters.end());
  assert(has_lock());

  return (decrement_holders() == 0);
};

void LockStage::notify_lock_obtained() { 
  assert(requesters.size() > 0);
  // Make sure that we never give a lock twice. Without this condition
  // this may happen in the following scenario:
  //    1) A scheduling begins to merge in a new lock queue
  //    2) An execution thread has finished executing a lock stage
  //    3) Execution thread pops the head
  //    4) The scheduling thread merges and grants lock
  //    5) Execution thread attempts to grant lock
  if (cmp_and_swap(&has_been_given_lock, 0, 1) == false) {
    return;
  }

  // notify every action within the lock stage.
  for (auto action_ptr : requesters) {
    action_ptr->notify_lock_obtained();
  } 
};

bool LockStage::has_lock() {
  return has_been_given_lock;
};
