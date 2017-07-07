#ifndef _MEMORY_POOL_INTERFACE_H_
#define _MEMORY_POOL_INTERFACE_H_

#include "batch/mutex_rw_guard.h"

#include <cassert>
#include <pthread.h>

// TODO: Make this a general queue and template it.
//
// Single consumer, single producer.
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
  pthread_rwlock_t lock;
  unsigned int cur_size;
public:
  MemEltQueue(): head(nullptr), tail(nullptr), cur_size(0) {
    pthread_rwlock_init(&lock, NULL);
  };

  void push_tail(MemElt* me) {
    me->next_elt = nullptr;

    MutexRWGuard g(&lock, LockType::exclusive);
    if (tail == nullptr) {
      head = me;
      tail = head;
    } else {
      tail->next_elt = me;
      tail = me;
    }
    cur_size ++;
  };

  MemElt* pop_head() {
    if (is_empty()) {
      return nullptr;
    }

    MutexRWGuard g(&lock, LockType::exclusive);
    auto tmp = head;
    head = head->next_elt;
    if (head == nullptr) {
      assert(tail == tmp);
      tail = nullptr;
    }

    cur_size --;
    tmp->next_elt = nullptr;
    return tmp;
  };
 
  bool is_empty() {
    MutexRWGuard g(&lock, LockType::shared);
    return (head == nullptr);
  };

  unsigned int size() {
    MutexRWGuard g(&lock, LockType::shared);
    return cur_size;
  };

  void free_all_memory() {
    MemElt* tmp;
    while ((tmp = pop_head()) != nullptr) {
      delete tmp;
    }
  };

  ~MemEltQueue() { free_all_memory(); };
};

// single allocator, single freer
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
  virtual unsigned int available_elts();

  virtual ~MemoryPool();
};

#include "batch/memory_pool_inter_impl.h"

#endif // _MEMORY_POOL_INTERFACE_H_
