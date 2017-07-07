#ifndef _MEMORY_POOLS_IMPL_H_
#define _MEMORY_POOLS_IMPL_H_

#include "batch/memory_pools.h"

LockStage* LockStagePool::alloc_and_initialize(
    LockStage::RequestingActions reqs,
    LockType lt) {
  return new (ls_memory_pool.alloc()) LockStage(reqs, lt);
};

LockStage* LockStagePool::alloc_and_initialize() {
  return new (ls_memory_pool.alloc()) LockStage();
};

#endif // _MEMORY_POOLS_IMPL_H_
