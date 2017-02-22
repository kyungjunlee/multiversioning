#include "batch/lock_stage.h"
#include "util.h"
#include <cassert>

LockStage::LockStage(): 
    holders(0),
    l_type(LockType::shared),
    requesters({})
  {};

LockStage::LockStage(
      RequestingActions requesters,
      LockType lt):
    holders(requesters.size()),
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

const LockStage::RequestingActions& LockStage::get_requesters() const {
  return requesters;
}

uint64_t LockStage::get_holders() const {
  return holders;
}
