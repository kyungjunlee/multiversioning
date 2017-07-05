#ifndef _STAT_MAP_IMPL_H_
#define _STAT_MAP_IMPL_H_

#include <cassert>

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair::KeyValuePair():
  KeyValuePair(0, 0)
{};

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair::KeyValuePair(KeyType kt):
  KeyValuePair(kt, 0)
{};

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair::KeyValuePair(
    KeyType kt,
    ValueType vt):
  key(kt),
  value(vt)
{};

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair::operator==(
    const KeyValuePair &other) const {
  return key == other.key;
};

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair::operator<(
    const KeyValuePair &other) const {
  return key < other.key;
};

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair::operator>(
    const KeyValuePair &other) const {
  return key > other.key;
};

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::is_empty() const {
  return contents.is_empty();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::is_full() const {
  return contents.is_full();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::insert(
    KeyType key,
    ValueType val) {
  assert(contents.find({key}) == nullptr);

  return contents.insert({key, val});
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
void StaticMap<KeyType, ValueType, maximal_size>::sort() {
  contents.sort();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
const ValueType* StaticMap<KeyType, ValueType, maximal_size>::find(
    const KeyType& key) const {
  return &(contents.find({key})->value);
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
bool StaticMap<KeyType, ValueType, maximal_size>::contains(
    const KeyType& key) const {
  return contents.contains({key});
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
unsigned int StaticMap<KeyType, ValueType, maximal_size>::capacity() const {
  return contents.capacity();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
unsigned int StaticMap<KeyType, ValueType, maximal_size>::size() const {
  return contents.size();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
typename StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair*
StaticMap<KeyType, ValueType, maximal_size>::begin() {
  return contents.begin();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
const typename StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair*
StaticMap<KeyType, ValueType, maximal_size>::begin() const {
  return contents.begin();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
typename StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair*
StaticMap<KeyType, ValueType, maximal_size>::end() {
  return contents.end();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
const typename StaticMap<KeyType, ValueType, maximal_size>::KeyValuePair*
StaticMap<KeyType, ValueType, maximal_size>::end() const {
  return contents.end();
}

template <
  typename KeyType,
  typename ValueType,
  unsigned int maximal_size>
void StaticMap<KeyType, ValueType, maximal_size>::clear() {
  contents.clear();
}

#endif // _STAT_MAP_IMPL_H_
