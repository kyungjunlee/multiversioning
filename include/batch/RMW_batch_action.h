#ifndef RMW_BATCH_ACTION_H_
#define RMW_BATCH_ACTION_H_

#include "batch/batch_action.h"
#include "batch/db_storage_interface.h"
#include "batch/stat_vec.h"
#include "batch/stat_map.h"

#include <unordered_map>

// RMWBatchAction
//
//    RMWBatchAction implements the simplest kind of RMW action which
//    reads all of the records within read and write sets and increments
//    by 1 the value found within the records of write set. 
//
//    Note that the values read are stored intermittently to simulate
//    the possibility of an abort of an action.
class RMWBatchAction : public BatchAction {
private:
  // TODO:
  //    As in the batch action interface, we must change the 60 to something
  //    less arbitrary.
  typedef StaticMap<RecordKey, IDBStorage::RecordValue, 60> TmpReads;

  TmpReads tmp_reads; 
  void add_to_tmp_reads(RecordKey rk);
  void do_reads(IDBStorage* db);
  void do_writes(IDBStorage* db);
public:
  RMWBatchAction(txn* t);

  virtual void add_read_key(RecordKey rk) override;
  virtual void add_write_key(RecordKey rk) override;
  virtual void finish_creating_action() override;

  virtual void Run(IDBStorage* db) override;
};

#endif // RMW_BATCH_ACTION_H_
