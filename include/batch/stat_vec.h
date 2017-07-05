#ifndef STATIC_VECTOR_H_
#define STATIC_VECTOR_H_

template <typename type, unsigned int maximal_size>
class StaticVector {
private:
  unsigned int next_elt;
  bool sorted;

  type contents[maximal_size];
public:
  StaticVector(): StaticVector({}) {};
  StaticVector(std::initializer_list<type> elts):
    next_elt(0),
    sorted(false) 
  {
    for (auto& t : elts) {
      insert(t);
    };
  };

  bool is_full();
  // returns true on success and false otherwise (container full);
  bool insert(type elt);
  void sort();
  // make sure to sort before finding or checking for containment.
  // Otherwise linear search instead of binary search is used!
  const type* find(const type& elt) const;
  bool contains(const type& elt) const;
  unsigned int capacity() const;
  unsigned int size() const;
  type* begin();
  const type* begin() const;
  type* end();
  const type* end() const;
  void clear();
};

#include "batch/stat_vec_impl.h"

#endif // STATIC_VECTOR_H_
