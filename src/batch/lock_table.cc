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
  auto smallest_it = blt.lock_table.begin();
  auto biggest_it = blt.lock_table.rbegin();
  if (smallest_it == blt.lock_table.end() || 
      biggest_it == blt.lock_table.rend()) {
    return;
  }

  merge_batch_table_for(
      blt, 
      smallest_it->first, 
      biggest_it->first);
}

void LockTable::merge_batch_table_for(
    BatchLockTable& blt,
    const RecordKey& from,
    const RecordKey& to) {
  
  LockTableType::iterator lt_it;
  BatchLockTable::LockTableType::iterator lo, hi;
  lo = blt.lock_table.lower_bound(from);
  hi = blt.lock_table.upper_bound(to);

  // merge queue by queue
  for (auto& elt = lo; elt != hi; elt++) {
    if (!memory_preallocated) {
      // this defualt-constructs the lock queue without any move or copy instructions.
      lock_table[elt->first];
    }

    lt_it = lock_table.find(elt->first);
    assert(lt_it != lock_table.end());

    auto head_blt = elt->second.peek_head();
    lt_it->second.merge_queue(&elt->second);

    // if the lock stage at the head has NOT been given the lock,
    // we should give it the lock. That means that we have merged into a queue 
    // that was empty and the execution thread must know that this stage
    // has the lock.
    auto head_pt = lt_it->second.peek_head();
    if (head_pt == nullptr) return;

    auto& head = *head_pt;
    if (head == *head_blt && 
        head->has_lock() == false) {
      head->notify_lock_obtained();
    }
  }
}

std::shared_ptr<LockStage> LockTable::get_head_for_record(RecordKey key) {
  auto elt = lock_table.find(key);
  assert(elt != lock_table.end());
  auto head_pt = elt->second.peek_head();

  return head_pt == nullptr ? nullptr : *head_pt;
};

void LockTable::pass_lock_to_next_stage_for(RecordKey key) {
  auto elt = lock_table.find(key);
  assert(elt != lock_table.end());

  // Lock Queue
  auto& lq = elt->second;
  // Pop the old lock stage
  lq.pop_head();

  // notify the new stage if there is one present.
  auto head_pt = lq.peek_head();
  if (head_pt != nullptr) {
    (*head_pt)->notify_lock_obtained();
  }
}

void LockTable::allocate_mem_for(RecordKey key) {
  auto insert_res = lock_table.insert(
      std::make_pair(key, LockQueue()));  
  assert(insert_res.second);
};

BatchLockTable::BatchLockTable() {}

void BatchLockTable::insert_lock_request(IBatchAction* req) {
  auto add_request = [this, &req](
      IBatchAction::RecordKeySet* set, LockType typ) {
    for (auto& i : *set) {
      BatchLockQueue& blq = lock_table.emplace(i, BatchLockQueue()).first->second;
      if (blq.is_empty() || 
          ((*blq.peek_tail())->add_to_stage(req, typ) == false)) {
        // insertion into the stage failed. Make a new stage and add it in.
        blq.non_concurrent_push_tail(std::move(
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
