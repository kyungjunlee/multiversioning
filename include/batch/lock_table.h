#ifndef _LOCK_TABLE_H_
#define _LOCK_TABLE_H_

#include "batch/batch_action.h"
#include "batch/lock_queue.h"

#include <unordered_map>
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
//    Currently we only allow a single scheduling thread to merge a BatchLockTable into
//    a LockTable and a scheduling thread will block within merge_batch_table until
//    it is able to merge. 
//
//    TODO: this behavior may be changed to (for instance) add
//    the BLTs to a queue. Then, a thread does not exit merge_batch_table until said
//    queue is empty and all the other scheduling threads may continue working on schedules.
//    This should only be done after we have seen that this reduced throughput by a lot.
class LockTable {
public:
  typedef std::unordered_map<BatchAction::RecKey, std::shared_ptr<LockQueue>> LockTableType;
protected:
  LockTableType lock_table;
  std::mutex merge_batch_table_mutex;
public:
  // TODO:
  //    Make sure that the lock table has all of the necessary queues allocated up front
  //    before anything merges into it. That way we can avoid emplacing new queues
  //    at run time!
  LockTable();
  void merge_batch_table(BatchLockTable& blt);
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
  typedef std::unordered_map<BatchAction::RecKey, std::shared_ptr<BatchLockQueue>> LockTableType;
protected:
  LockTableType lock_table;
public:
  BatchLockTable();
  void insert_lock_request(std::shared_ptr<BatchAction> request);
  const LockTableType& get_lock_table_data();

  friend class LockTable;
};

#endif // _LOCK_TABLE_H_
