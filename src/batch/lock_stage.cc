#include "batch/lock_stage.h"
#include "util.h"
#include <cassert>

LockStage::LockStage(): 
    holders(0),
    next_stage(nullptr),
    l_type(LockType::shared),
    requesters({})
  {};

LockStage::LockStage(
      RequestingActions requesters,
      LockType lt,
      LockStage* ns):
    holders(requesters.size()),
    next_stage(ns),
    l_type(lt),
    requesters(requesters)
  {
    assert(!(lt == LockType::exclusive && requesters.size() > 1));
  };

bool LockStage::add_to_stage(Action_spt txn, LockType lt) {
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
}

uint64_t LockStage::decrement_holders() {
  return fetch_and_decrement(&holders);
}

bool LockStage::set_next_stage(LockStage* ns) {
  return cmp_and_swap((uint64_t*) &next_stage, 0, (uint64_t) ns); 
}

LockStage* LockStage::get_next_stage() {
  return next_stage; 
}

const LockStage::RequestingActions& LockStage::get_requesters() const {
  return requesters;
}

uint64_t LockStage::get_holders() const {
  return holders;
}
