#ifndef STATIC_VECTOR_H_
#define STATIC_VECTOR_H_

template <typename type, unsigned int maximal_size>
class StaticVector {
public:
  type contents[maximal_size];
  unsigned int next_elt;
  bool sorted;

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
  // returns true on success and false otherwise
  bool insert(type elt);
  void sort();
  type* find(const type& elt);
  bool contains(const type& elt);
  unsigned int capacity() const;
  unsigned int size() const;
  type* begin();
  type* end();
  void clear();
};

#include "batch/stat_vec_impl.h"

#endif // STATIC_VECTOR_H_
