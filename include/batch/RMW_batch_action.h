#ifndef RMW_BATCH_ACTION_H_
#define RMW_BATCH_ACTION_H_

#include "batch/batch_action.h"
#include "batch/db_storage_interface.h"

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
    typedef std::unordered_map<RecordKey, IDBStorage::RecordValue> TmpReadMap;
    TmpReadMap tmp_reads; 

    void add_to_tmp_reads(RecordKey rk);
    void do_reads(IDBStorage* db);
    void do_writes(IDBStorage* db);
  public:
    RMWBatchAction(txn* t);

    virtual void add_read_key(RecordKey rk) override;
    virtual void add_write_key(RecordKey rk) override;

    virtual void Run(IDBStorage* db) override;
};

#endif // RMW_BATCH_ACTION_H_
