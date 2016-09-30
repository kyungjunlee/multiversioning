#ifndef                 LOCKING_WORKER_HH_
#define                 LOCKING_WORKER_HH_

#include <table.h>
#include <lock_manager.h>
#include <concurrent_queue.h>
#include <pthread.h>
#include <cpuinfo.h>
#include <runnable.hh>
#include <record_buffer.h>
#include <mcs.h>
#include <insert_buf_mgr.h>
#include <table_mgr.h>
#include <mcs_rw.h>
#include <tpcc.h>


struct locking_action_batch {
  uint32_t batchSize;
  locking_action **batch;
};

typedef SimpleQueue<locking_action_batch> locking_queue;

struct locking_worker_config {
  LockManager *mgr;
  locking_queue *inputQueue;
  locking_queue *outputQueue;  
  int cpu;
  uint32_t maxPending;
  table_mgr *tbl_mgr;
};

class locking_worker : public Runnable {
private:
  locking_worker_config config;
  locking_action *m_queue_head;                  // Head of queue of waiting txns
  locking_action *m_queue_tail;                  // Tail of queue of waiting txns
  int         m_num_elems;                    // Number of elements in the queue
  mcs_mgr *mgr;
  insert_buf_mgr *insert_mgr;
  volatile uint32_t m_num_done;
  lck_key_allocator *key_alloc;

  RecordBuffers *bufs;
  
  // Worker thread function
  virtual void WorkerFunction();

  void Enqueue(locking_action *txn);

  void RemoveQueue(locking_action *txn);

  void CheckReady();

  void TryExec(locking_action *txn);

  void DoExec(locking_action *txn);
    
  uint32_t QueueCount(locking_action *txn);

  void give_locks(locking_action *txn);
  void take_locks(locking_action *txn);

protected:    
  virtual void StartWorking();
  
  virtual void Init();

public:
  void* operator new(std::size_t sz, int cpu) {
    return alloc_mem(sz, cpu);
  }

  locking_worker(locking_worker_config config, RecordBuffersConfig rb_conf);
    
  uint32_t NumProcessed() {
    return m_num_done;
  }
};

#endif           // LOCKING_WORKER_HH_
