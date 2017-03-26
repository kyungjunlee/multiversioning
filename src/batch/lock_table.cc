#include "batch/lock_table.h"
#include "batch/lock_types.h"

#include <cassert>

LockTable::LockTable(): memory_preallocated(false) {};

LockTable::LockTable(DBStorageConfig db_conf): 
  memory_preallocated(true) 
{
  for (auto& table_conf : db_conf.tables_definitions) {
    for (uint64_t i = 0; i < table_conf.num_records; i++) {
      allocate_mem_for({i, table_conf.table_id});
    }
  }
} 

void LockTable::merge_batch_table(BatchLockTable& blt) {
  std::lock_guard<std::mutex> lock(merge_batch_table_mutex);

  // now we are the only thread merging into the global schedule.
  LockTableType::iterator lt_it;
  // merge queue by queue
  for (auto& elt : blt.lock_table) {
    if (memory_preallocated) {
      // if memory has been preallocated, make sure that we never attempt
      // to access non-existant record
      lt_it = lock_table.find(elt.first);
      assert(lt_it != lock_table.end());
    } else {
      // otherwise making records is alright
      lt_it = lock_table.emplace(elt.first, std::make_shared<LockQueue>()).first;
    }

    auto head_blt = elt.second->peek_head();
    lt_it->second->merge_queue(elt.second.get());

    // if the lock stage at the head has NOT been given the lock,
    // we should give it the lock. That means that we have merged into a queue 
    // that was empty and the execution thread must know that this stage
    // has the lock.
    auto head = lt_it->second->peek_head();
    barrier();
    if (head == head_blt && head->has_lock() == false) {
      head->notify_lock_obtained();
    }
  }
}

std::shared_ptr<LockStage> LockTable::get_head_for_record(RecordKey key) {
  auto elt = lock_table.find(key);
  assert(elt != lock_table.end());

  return elt->second->is_empty() ? nullptr : elt->second->peek_head();
};

void LockTable::pass_lock_to_next_stage_for(RecordKey key) {
  auto elt = lock_table.find(key);
  assert(elt != lock_table.end());

  // Lock Queue
  auto lq = elt->second;
  // Pop the old lock stage
  lq->pop_head();

  // notify the new stage if there is one present.
  if (!lq->is_empty()) {
    lq->peek_head()->notify_lock_obtained();
  }
}

void LockTable::allocate_mem_for(RecordKey key) {
  auto insert_res = lock_table.insert(
      std::make_pair(key, std::make_shared<LockQueue>()));  
  assert(insert_res.second);
};

BatchLockTable::BatchLockTable() {}

void BatchLockTable::insert_lock_request(std::shared_ptr<IBatchAction> req) {
  auto add_request = [this, &req](
      IBatchAction::RecordKeySet* set, LockType typ) {
    std::shared_ptr<BatchLockQueue> blq;
    for (auto& i : *set) {
      blq = lock_table.emplace(i, std::make_shared<BatchLockQueue>()).first->second;
      if (blq->is_empty() || 
          blq->peek_tail()->add_to_stage(req, typ) == false) {
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
