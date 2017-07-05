#ifndef _STATIC_MAP_H_
#define _STATIC_MAP_H_

#include "batch/stat_vec.h"

// super simple wrapper around stat_vec allowing pairs.
template <
  typename KeyType, 
  typename ValueType, 
  unsigned int maximal_size>
class StaticMap {
private:
  struct KeyValuePair {
    KeyType key;
    ValueType value;

    KeyValuePair(): key(0), value(0) {};
    KeyValuePair(KeyType kt): key(kt), value(0) {};
    KeyValuePair(KeyType kt, ValueType vt): key(kt), value(vt) {};

    bool operator==(const KeyValuePair &other) const {return key == other.key;};
    bool operator<(const KeyValuePair &other) const {return key < other.key;};
    bool operator>(const KeyValuePair &other) const {return key > other.key;};
  };

  StaticVector<KeyValuePair, maximal_size> contents;
public:

  bool is_full() {return contents.is_full();};
  // returns true on success and false on failure (container full).
  bool insert(KeyType key, ValueType val) {return contents.insert({key, val});};

  // make sure to sort before finding or checking for containment.
  // Otherwise linear search instead of binary search is used!
  void sort() {contents.sort();};
  const ValueType* find(const KeyType& key) const {
    return &contents.find({key})->value;};
  bool contains(const KeyType& key) const {return contents.contains({key});};
  unsigned int capacity() const {return contents.capacity();};
  unsigned int size() const {return contents.size();};
  KeyValuePair* begin() {return contents.begin();};
  const KeyValuePair* begin() const {return contents.begin();};
  KeyValuePair* end() {return contents.end();};
  const KeyValuePair* end() const {return contents.end();};
  void clear() {contents.clear();};
};

#endif // _STATIC_MAP_H_
