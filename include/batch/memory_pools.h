#ifndef _MEMORY_POOLS_H_
#define _MEMORY_POOLS_H_

#include "batch/memory_pool_interface.h"
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

#include "batch/memory_pools_impl.h"

#endif // _MEMORY_POOLS_H_