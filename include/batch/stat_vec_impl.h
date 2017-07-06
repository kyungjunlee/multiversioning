#ifndef STATIC_VECTOR_IMPL_H
#define STATIC_VECTOR_IMPL_H

#include <algorithm>
#include <cassert>

template <typename type, unsigned int maximal_size>
bool StaticVector<type, maximal_size>::is_empty() const {
  return (next_elt == 0);
};

template <typename type, unsigned int maximal_size>
bool StaticVector<type, maximal_size>::is_full() const {
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
bool StaticVector<type, size>::push_back(type elt) {
  auto result = insert(elt);
  assert(result);
  return result;
};

template <typename type, unsigned int size>
void StaticVector<type, size>::sort() {
  if (sorted) return;

  std::sort(std::begin(contents), &contents[next_elt], std::less<type>());
  sorted = true;
};

template <typename type, unsigned int size>
bool StaticVector<type, size>::contains(const type& elt) const {
  return (nullptr != find(elt));
};

template <typename type, unsigned int maximal_size>
const type* StaticVector<type, maximal_size>::find(const type& elt) const {
  if (next_elt == 0) return nullptr;
  if (sorted == false) {
    // linear search
    for (unsigned int i = 0; i < size(); i++) {
      if (elt == contents[i]) {
        return &contents[i];
      }
    };

    return nullptr;
  }

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
const type* StaticVector<type, size>::begin() const {
  return &contents[0];
};

template <typename type, unsigned int size>
type* StaticVector<type, size>::end() {
  return &contents[next_elt];
};

template <typename type, unsigned int size>
const type* StaticVector<type, size>::end() const {
  return &contents[next_elt];
};

template <typename type, unsigned int size>
void StaticVector<type, size>::clear() {
  next_elt = 0;
  sorted = false;
};

template <typename type, unsigned int maximal_size>
type& StaticVector<type, maximal_size>::operator[](std::size_t idx) {
  assert(idx < size());

  return contents[idx];
}

template <typename type, unsigned int maximal_size>
const type& StaticVector<type, maximal_size>::operator[](
    std::size_t idx) const {
  assert(idx < size());

  return contents[idx];
}
#endif // STATIC_VECTOR_IMPL_H
