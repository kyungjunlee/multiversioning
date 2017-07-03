#ifndef BATCH_RECORD_KEY_H_
#define BATCH_RECORD_KEY_H_

#include <city.h>
#include <cstdint>
#include <functional>
#include <utility>

// RecordKey
//
//    The representation of record key index used to specify a record.
//    The reason for the existance of this instead of using a bare integer
//    is the existence of multiple tables.
class RecordKey {
public:
  static constexpr uint64_t DEFAULT_TABLE_ID = 0;

  uint64_t key;
  uint64_t table_id;

  RecordKey();
  RecordKey(uint64_t key);
  RecordKey(uint64_t key, uint64_t table_id);

  bool operator==(const RecordKey &other) const;
  bool operator<(const RecordKey &other) const;
  bool operator>(const RecordKey &other) const;
};

namespace std {
  template <>
  struct hash<RecordKey> {
    size_t operator() (const RecordKey &k) const {
      return Hash128to64(std::make_pair(k.key, k.table_id)); 
    }
  };
}

#endif // BATCH_RECORD_KEY_H_
