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

// TODO:
//    Describe this class. IT IS NON CONCURRENT!!!
//    NO INSERTIONS
class IDBStorage {
public:
  typedef uint64_t RecordValue;

  virtual RecordValue read_record_value(RecordKey key) = 0;
  virtual void write_record_value(RecordKey key, RecordValue value) = 0;
};

#endif // BATCH_DB_STORAGE_INTERFACE_H_
