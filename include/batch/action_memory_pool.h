#ifndef _ACTION_MEMORY_POOL_H_
#define _ACTION_MEMORY_POOL_H_

#include "batch/memory_pool.h"
#include "test/test_txn.h"

template <class ActionClass>
class ActionMemoryPool : public MemoryPool<ActionClass> {
private:
  MemoryPool<TestTxn> txn_memory_pool;

  using MemoryPool<ActionClass>::alloc;
public:
  ActionClass* alloc_and_initialize();
  void free(void* act) override;

  virtual ~ActionMemoryPool();
};

#include "batch/action_memory_pool_impl.h"

#endif //_ACTION_MEMORY_POOL_H_
