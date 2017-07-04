#ifndef _MEMORY_POOL_INTERFACE_H_
#define _MEMORY_POOL_INTERFACE_H_

#include <cassert>

// TODO: Make this a general queue and template it.
class MemEltQueue {
private:
  struct MemElt {
    struct MemElt* next_elt;
    char* memory;

    MemElt(unsigned int byte_size) {
      next_elt = nullptr;
      memory = new char[byte_size]; 
    };

    ~MemElt() {
      next_elt = nullptr;
     delete[] memory; 
    };
  };

  MemElt* head;
  MemElt* tail;
public:
  MemEltQueue(): head(nullptr), tail(nullptr) {};

  void push_tail(MemElt* me) {
    me->next_elt = nullptr;

    if (is_empty()) {
      head = me;
      tail = head;
    } else {
      tail->next_elt = me;
      tail = me;
    }
  };

  MemElt* pop_head() {
    if (is_empty()) {
      return nullptr;
    }

    auto tmp = head;
    head = head->next_elt;
    if (head == nullptr) {
      assert(tail == tmp);
      tail = nullptr;
    }

    tmp->next_elt = nullptr;
    return tmp;
  };
  
  bool is_empty() {
    return (head == nullptr);
  };

  void free_all_memory() { 
    MemElt* tmp;
    while ((tmp = pop_head()) != nullptr) {
      delete tmp;
    }
  };

  ~MemEltQueue() { free_all_memory(); };
}; 

// TODO:
//
//    Does this have to be multithreaded in any way? Probably so! 
//    The above queue might have to go, but consider MR queue.
template <class AllocatedClass>
class MemoryPool {
protected:
  MemEltQueue free_list;
  MemEltQueue allocated_list;
  
  virtual void allocate_new_element();
public:
  virtual void* alloc();
  virtual void free(void* elt);

  virtual void preallocate_memory(unsigned int number_of_elts);

  virtual ~MemoryPool();
};

#include "batch/memory_pool_inter_impl.h"

#endif // _MEMORY_POOL_INTERFACE_H_
