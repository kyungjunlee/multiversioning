#include "batch/lock_table.h"
#include "batch/lock_types.h"

#include <cassert>

LockTable::LockTable() {} 

void LockTable::merge_batch_table(BatchLockTable& blt) {
  std::lock_guard<std::mutex> lock(merge_batch_table_mutex);

  // now we are the only thread merging into the global schedule.
  LockTableType::iterator lq;
  for (auto& elt : blt.lock_table) {
    // merge queue by queue
 
    // TODO: This should become uncommented when we allocate queues up front and the 
    // emplace code should be deleted!
    // lq = lock_table.find(elt.first);
    // assert(lq != lock_table.end());
    //
    //
    // For now, we create queues on the fly as we need them.
    lq = lock_table.emplace(elt.first, std::make_shared<LockQueue>()).first; 
    lq->second->merge_queue(elt.second.get());
  }
}

BatchLockTable::BatchLockTable() {}

void BatchLockTable::insert_lock_request(std::shared_ptr<BatchAction> req) {
  auto add_request = [this, &req](BatchAction::RecSet* set, LockType typ) {
    std::shared_ptr<BatchLockQueue> blq;
    for (auto& i : *set) {
      blq = lock_table.emplace(i, std::make_shared<BatchLockQueue>()).first->second;
      if (blq->peek_tail() == nullptr || 
          blq->peek_tail()->add_to_stage(req, typ) == false) {
        // insertion into the stage failed. Make a new stage and add it in.
        blq->non_concurrent_push_tail(std::move(LockStage({req}, typ))); 
      }
    }
  };
  
  add_request(req->get_writeset_handle(), LockType::exclusive);  
  add_request(req->get_readset_handle(), LockType::shared);  
}

const BatchLockTable::LockTableType& BatchLockTable::get_lock_table_data() {
  return lock_table; 
}
