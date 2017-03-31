#ifndef BATCH_DB_STORAGE_INTERFACE_H_
#define BATCH_DB_STORAGE_INTERFACE_H_

#include "batch/record_key.h"

#include <vector>

struct BatchTableConfig {
  uint64_t table_id;
  uint64_t num_records;
};

struct DBStorageConfig {
  std::vector<BatchTableConfig> tables_definitions;
};

// IDBStorage
//
//    IDBStorage represents the in-memory store of the database. 
//    It assumes that the number of records within the database does
//    not change over the lifetime of the database. DBStorage is non
//    thread safe in the meaning that reads and writes must be coordinated
//    on a higher level to guarantee no cinflicting writes/reads.
class IDBStorage {
public:
  typedef uint64_t RecordValue;

  virtual RecordValue read_record_value(RecordKey key) = 0;
  virtual void write_record_value(RecordKey key, RecordValue value) = 0;
};

#endif // BATCH_DB_STORAGE_INTERFACE_H_
