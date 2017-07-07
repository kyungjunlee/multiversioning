#ifndef _LOCK_STAGE_MEMORY_POOL_H_
#define _LOCK_STAGE_MEMORY_POOL_H_

#include "batch/lock_stage.h"

class LockStagePool : public MemoryPool<LockStage> {
private:
  MemoryPool<LockStage> ls_memory_pool;

  using MemoryPool<LockStage>::alloc;
public:
  LockStage* alloc_and_initialize(
      LockStage::RequestingActions reqs,
      LockType lt);
  LockStage* alloc_and_initialize();
};

#include "batch/memory_pools_impl.h"

#endif // _MEMORY_POOLS_H_
