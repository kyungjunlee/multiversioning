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

    KeyValuePair();
    KeyValuePair(KeyType kt);
    KeyValuePair(KeyType kt, ValueType vt);

    bool operator==(const KeyValuePair &other) const;
    bool operator<(const KeyValuePair &other) const;
    bool operator>(const KeyValuePair &other) const;
  };

  StaticVector<KeyValuePair, maximal_size> contents;

public:
  bool is_empty() const;
  bool is_full() const;
  // returns true on success and false on failure (container full).
  //
  // NOTE:
  //    Insertion occurs only if an element with "key" did not
  //    exist before. Otherwise an exception is thrown.
  bool insert(KeyType key, ValueType val);

  // make sure to sort before finding or checking for containment.
  // Otherwise linear search instead of binary search is used!
  void sort();
  const ValueType* find(const KeyType& key) const;
  bool contains(const KeyType& key) const;

  unsigned int capacity() const;
  unsigned int size() const;

  KeyValuePair* begin();
  const KeyValuePair* begin() const;
  KeyValuePair* end();
  const KeyValuePair* end() const;

//  const KeyType& lowest_key() const;
//  const KeyType& highest_key() const;
//
//  // NOTE:
//  //   0. The map MUST be sorted before these are called.
//  //      Otherwise the pointers returned have no meaning.
//  const KeyValuePair* lower_bound(const KeyType& key);
//  const KeyValuePair* upper_bound(const KeyType& key);

  void clear();
};

#include "batch/stat_map_impl.h"

#endif // _STATIC_MAP_H_
