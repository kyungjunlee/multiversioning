#ifndef _MEMORY_POOLS_IMPL_H_
#define _MEMORY_POOLS_IMPL_H_

#include "batch/memory_pools.h"

template <class ActionClass>
ActionClass* ActionMemoryPool<ActionClass>::alloc_and_initialize() {
  void* action_memory = MemoryPool<ActionClass>::alloc();
  void* txn_memory = txn_memory_pool.alloc();

  return new (action_memory) ActionClass(new (txn_memory) TestTxn());
};

template <class ActionClass>
void ActionMemoryPool<ActionClass>::free(void* act) {
  txn_memory_pool.free((void*) ((ActionClass*) act)->get_transaction());
  
  MemoryPool<ActionClass>::free(act);
};

template <class ActionClass>
ActionMemoryPool<ActionClass>::~ActionMemoryPool() {};

#endif // _MEMORY_POOLS_IMPL_H_
