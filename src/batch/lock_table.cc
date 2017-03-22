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
    auto head_blt = *elt.second->peek_head();

    lq = lock_table.emplace(elt.first, std::make_shared<LockQueue>()).first; 
    lq->second->merge_queue(elt.second.get());

    // if the lock stage at the head has NOT been given the lock,
    // we should give it the lock. That means that we have merged into a queue 
    // that was empty and the execution thread must know that this stage
    // has the lock.
    auto head = *lq->second->peek_head();
    barrier();
    if (head == head_blt && head->has_lock() == false) {
      head->notify_lock_obtained();
    }
  }
}

std::shared_ptr<LockStage> LockTable::get_head_for_record(RecordKey key) {
  auto elt = lock_table.find(key);
  assert(elt != lock_table.end());

  return *elt->second->peek_head();
};

void LockTable::pass_lock_to_next_stage_for(RecordKey key) {
  auto elt = lock_table.find(key);
  assert(elt != lock_table.end());

  // Lock Queue
  auto lq = elt->second;
  // Pop the old lock stage
  auto poppedElt = lq->try_pop_head();
  assert(poppedElt != nullptr);

  // notify the new stage if there is one present.
  auto new_lock_stage_ptr = *lq->peek_head();
  if (new_lock_stage_ptr != nullptr) {
    new_lock_stage_ptr->notify_lock_obtained();
  }
}

BatchLockTable::BatchLockTable() {}

void BatchLockTable::insert_lock_request(std::shared_ptr<BatchActionInterface> req) {
  auto add_request = [this, &req](
      BatchActionInterface::RecordKeySet* set, LockType typ) {
    std::shared_ptr<BatchLockQueue> blq;
    for (auto& i : *set) {
      blq = lock_table.emplace(i, std::make_shared<BatchLockQueue>()).first->second;
      if (blq->peek_tail() == nullptr || 
          (*blq->peek_tail())->add_to_stage(req, typ) == false) {
        // insertion into the stage failed. Make a new stage and add it in.
        blq->non_concurrent_push_tail(std::move(
              std::make_shared<LockStage>(LockStage({req}, typ))));
      }
    }
  };
  
  add_request(req->get_writeset_handle(), LockType::exclusive);  
  add_request(req->get_readset_handle(), LockType::shared);  
}

const BatchLockTable::LockTableType& BatchLockTable::get_lock_table_data() {
  return lock_table; 
}
