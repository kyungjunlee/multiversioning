#ifndef STATIC_VECTOR_IMPL_H
#define STATIC_VECTOR_IMPL_H

#include <algorithm>

template <typename type, unsigned int maximal_size>
bool StaticVector<type, maximal_size>::is_full() {
  return (maximal_size == next_elt);
};

template <typename type, unsigned int size>
bool StaticVector<type, size>::insert(type elt) {
  if (is_full()) {
    return false;
  }

  contents[next_elt++] = elt;
  sorted = false;
  return true;
};

template <typename type, unsigned int size>
void StaticVector<type, size>::sort() {
  std::sort(std::begin(contents), &contents[next_elt], std::less<type>());
  sorted = true;
};

template <typename type, unsigned int size>
bool StaticVector<type, size>::contains(const type& elt) {
  return (nullptr != find(elt));
};

template <typename type, unsigned int size>
type* StaticVector<type, size>::find(const type& elt) {
  if (next_elt == 0) return nullptr;
  if (sorted == false) sort();

  // sorted array. We may use binary search
  int lo, hi, cur;
  hi = next_elt - 1;
  lo = 0;
  cur = 0;

  while (hi >= lo) {
    cur = lo + (hi - lo) / 2;
    if (contents[cur] > elt) {
      hi = cur - 1;
    } else if (contents[cur] < elt) {
      lo = cur + 1;
    } else {
      return &contents[cur];
    }
  };

  return nullptr;
};

template <typename type, unsigned int maximal_size>
unsigned int StaticVector<type, maximal_size>::capacity() const {
  return maximal_size;
};

template <typename type, unsigned int maximal_size>
unsigned int StaticVector<type, maximal_size>::size() const {
  return next_elt;
};

template <typename type, unsigned int size>
type* StaticVector<type, size>::begin() {
  return &contents[0];
};

template <typename type, unsigned int size>
type* StaticVector<type, size>::end() {
  return &contents[next_elt];
};

template <typename type, unsigned int size>
void StaticVector<type, size>::clear() {
  next_elt = 0;
  sorted = false;
};

#endif // STATIC_VECTOR_IMPL_H
