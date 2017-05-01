#ifndef MAKE_STATICALLY_ALLOCATED_H_
#define MAKE_STATICALLY_ALLOCATED_H_

#include "static_memory_pool.h"

template <class T>
class StatAlloc : public T {
private:

public:
  void* operator new(std::size_t size) {
    
  }
};

#endif // MAKE_STATICALLY_ALLOCATED_H_
