#ifndef _LOCK_TABLE_H_
#define _LOCK_TABLE_H_

#include "batch/batch_action_interface.h"
#include "batch/db_storage_interface.h"
#include "batch/lock_queue.h"
#include "batch/record_key.h"

#include <unordered_map>
#include <map>
#include <mutex>

// TODO:
//    overwrite the new/delete operators when we figure out memory allocators...
class BatchLockTable;

// LockTable
//    
//    LockTable is very similar to the traditional lock table in locked systems. It 
//    contains lock stage queues (LockQueues) for every record in the database. The only
//    way to add elements to lock table is to merge in a BatchLockTable which represents
//    transaction schedule of a batch.
//
//  NOTE:
//    This lock table implementation is NOT inherently multithreaded and precautions must
//    be taken to ensure that data is not corrupted. Notice also that merge_batch_table_for
//    may be called concurrently if at no point are the ranges overlapping across function
//    invocations.
class LockTable {
public:
  typedef std::unordered_map<RecordKey, LockQueue> LockTableType;

protected:
  LockTableType lock_table;
  bool memory_preallocated;

  void allocate_mem_for(RecordKey key);

public:
  LockTable();
  LockTable(DBStorageConfig db_conf);
  void merge_batch_table(BatchLockTable& blt);
  // from and to are both inclusive.
  void merge_batch_table_for(
      BatchLockTable& blt, 
      const RecordKey& from, 
      const RecordKey& to);
  
  std::shared_ptr<LockStage> get_head_for_record(RecordKey key);
  void pass_lock_to_next_stage_for(RecordKey key);
};

// BatchLockTable
//
//    BatchLockTable is very similar to the global LockTable, however it only contains
//    entries that correspond to those within a batch. Moreover, a batchLockTable is
//    only every operated on by a single thread, hence it is non-concurrent. 
//
//    BatchLockTables are used by scheduling threads for creating tables that may be
//    easily merged into the global LockTable.
class BatchLockTable {
public:
  typedef std::map<RecordKey, BatchLockQueue> LockTableType;

protected:
  LockTableType lock_table;

public:
  BatchLockTable();
  void insert_lock_request(std::unique_ptr<IBatchAction>&& request);
  const LockTableType& get_lock_table_data();

  friend class LockTable;
};

#endif // _LOCK_TABLE_H_
