#include "batch/db_storage.h"

#include <cassert>
#include <cstring>

DBStorage::DBStorage(DBStorageConfig db_conf) {
  preallocate_records(db_conf);
}

DBStorage::Data::iterator DBStorage::insert_elt(RecordKey key, RecordValue val) {
  auto insert_res = data.insert(
      std::make_pair(key, val));
  // assert that insertion was successful
  assert(insert_res.second);

  return insert_res.first;
}

DBStorage::RecordValue DBStorage::read_record_value(RecordKey key) {
  auto res = data.find(key);
  assert(res != data.end());

  return res->second;
};

void DBStorage::write_record_value(RecordKey key, RecordValue value) {
  auto elt = data.find(key);
  // we assume all of the records are inside the db from get go
  if (elt == data.end()) {
    assert(false);
  }
  
  // avoid using any writes of std::unordered_map to guarantee
  // that we are not changing the structure of the map.
  memcpy((void*) &elt->first, (const void*) &value, sizeof(RecordValue));
}

void DBStorage::preallocate_records(DBStorageConfig conf) {
  for (auto& table_conf : conf.tables_definitions) {
    for (uint64_t i = 0; i < table_conf.num_records; i++) {
      insert_elt({i, table_conf.table_id}, 0);
    }
  }
};
