#ifndef STATIC_ALLOCATOR_H_
#define STATIC_ALLOCATOR_H_

#include "batch/mutex_rw_guard.h"
#include "batch/static_memory_pool_interface.h"

#include <list>
#include <pthread>

template <class T>
class StatMemPool : public IStaticMemoryPool {
private:
  struct AllocElt {
    char[sizeof(T)] mem;
  };

  unsigned int chunk_size;
  std::list<AllocElt*> free_list;
  pthread_rwlock_t mutex; 
  unsigned int total_allocated_elts;

  void expand_pool(unsigned int num_elts) {
    total_allocated_elts += num_elts;

    for (unsigned int i = 0; i < num_elts; i++) {
      free_list.push_back(new AllocElt());
    }
  };

public:
  StatMemPool(unsigned int initial_elements): 
    IStaticMemoryPool(initial_elements),
    total_allocated_elts(0)
  {
    pthread_rwlock_init(&mutex, NULL);
    chunk_size = sizeof(T);
    expand_pool(initial_elements);
  };

  inline void* allocate(size_t size) {
    assert(size == chunk_size);
    MutexRWGuard(&mutex, LockType::exclusive);
    // expand by 10% of the initial number of elements. 
    if (free_list.empty()) expand_pool(this->initial_size * 0.1);
     
    return reinterpret_cast<void*>(free_list.pop_front());
  };

  inline void free(void* toDelete) {
    MutexRWGuard(&mutex, LockType::exclusive);
    free_list.push_back(reinterpret_cast<AllocElt> toDelete);
  };

  // make sure that all memory is deallocated.
  ~StatMemPool() {
    MutexRWGuard g(&mutex, LockType::shared);
    g.unlock();
    // wait until all of the elements have been returned
    while (true) {
      g.read_lock();
      if (free_list.size() == total_allocated_elts) {
        break;
      }
      g.unlock();
    }

    // free all of the memory.
    g.write_lock();
    while (free_list.empty() == false) {
      auto e = free_list.pop_front();
      delete e;
    } 
    g.unlock();
    
    pthread_rwlock_destroy(&lock);
  };
};

#endif //STATIC_ALLOCATOR_H_
