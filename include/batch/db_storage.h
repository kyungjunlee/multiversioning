#ifndef BATCH_DB_STORAGE_H_
#define BATCH_DB_STORAGE_H_

#include "batch/db_storage_interface.h"

#include <unordered_map>
// TODO:
//    Description
//    Preallocates all memory
class DBStorage : public IDBStorage {
private:
  typedef std::unordered_map<RecordKey, RecordValue> Data;

  Data data;
  Data::iterator insert_elt(RecordKey key, RecordValue value);
  void preallocate_records(DBStorageConfig db_conf);
public:
  typedef IDBStorage::RecordValue RecordValue;

  DBStorage(DBStorageConfig db_conf);

  // override IDBStorage
  RecordValue read_record_value(RecordKey key);
  void write_record_value(RecordKey key, RecordValue value);
};

#endif //BATCH_DB_STORAGE_H_
