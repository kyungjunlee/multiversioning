#include "batch/record_key.h"

constexpr uint64_t RecordKey::DEFAULT_TABLE_ID;

RecordKey::RecordKey(): RecordKey(-1, -1) {};
RecordKey::RecordKey(uint64_t key): RecordKey(key, DEFAULT_TABLE_ID) {};
RecordKey::RecordKey(uint64_t key, uint64_t table_id): key(key), table_id(table_id) {};

bool RecordKey::operator==(const RecordKey& other) const {
  return (key == other.key && table_id == other.table_id);
}

bool RecordKey::operator<(const RecordKey& other) const {
  // Order tables increasingly and order records by key within tables.
  return (table_id < other.table_id ||
          (table_id == other.table_id && key < other.key));
}

bool RecordKey::operator>(const RecordKey& other) const {
  // Order tables increasingly and order records by key within tables.
  return (table_id > other.table_id ||
          (table_id == other.table_id && key > other.key));
}
