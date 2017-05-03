#ifndef SCHEDULER_MANAGER_H_
#define SCHEDULER_MANAGER_H_

#include "batch/scheduler.h"
#include "batch/lock_table.h"
#include "batch/batched_input_queue.h"
#include "batch/scheduler_system.h"
#include "batch/scheduler_thread_manager.h"

#include <vector>
#include <memory>

// ThreadInputQueue
//
//    A single consumer, single producer queue used to pass batches
//    to scheduling threads to avoid contention on shared data structures.
//    The queue is filled by one of the scheduling threads when there 
//    is opportunity for it. Please see request_input for more 
//    information.
class ThreadInputQueue : public MSQueue<SchedulerThreadBatch> {
  private:
      using MSQueue<SchedulerThreadBatch>::merge_queue;
}; 

//  AwaitingBatch
//
//    An internal representation of all the data necessary for handing
//    a batch off to the execution threads. This is necessary because 
//    we maintain the ordering of batches and require as little 
//    contention on data structures as possible.
struct AwaitingBatch {
  uint64_t id;
  BatchLockTable blt;
  ExecutorThreadManager::ThreadWorkloads tw;  
};

// HandingQueue
//
//    A single consumer, single producer queue used to queue up scheduler
//    batches that cannot yet be handed over to execution threads because
//    of "former" batches still being processed. Each scheduling thread
//    has its own queue and there may be only one thread performing the 
//    handing off.
class HandingQueue : public MSQueue<AwaitingBatch> {
  private:
    using MSQueue<AwaitingBatch>::merge_queue;
};

struct AwaitingSchedulerBatches {
  AwaitingSchedulerBatches(unsigned int thread_number) {
    pending_queues.resize(thread_number);
  };

  std::vector<HandingQueue> pending_queues;
};

struct ThreadInputQueues {
  ThreadInputQueues(unsigned int thread_number) {
    queues.resize(thread_number);
  }

  std::vector<ThreadInputQueue> queues;
};

//  Scheduler Manager
//
//  Scheduler Manager is the actual implementation of Scheduling System and
//  Scheduler Thread Manager classes. The supervisor class has a handle to it
//  using a SchedulingSystem pointer and the scheduling threads have a handle
//  to it using the SchedulerThreadManager pointer.
class SchedulerManager : 
  public SchedulerThreadManager,
  public SchedulingSystem {
private:
  bool system_is_initialized();
  void create_threads();
public:
  ThreadInputQueues thread_input;
  uint64_t input_batch_id;
  pthread_rwlock_t input_lock;
  uint64_t handed_batch_id;
  AwaitingSchedulerBatches pending_batches;
  pthread_rwlock_t handing_lock;
  
  std::unique_ptr<InputQueue> iq;
	std::vector<std::shared_ptr<SchedulerThread>> schedulers;
  IGlobalSchedule* gs;

  SchedulerManager(
      SchedulingSystemConfig c,
      ExecutorThreadManager* exec);

  // implementing the SchedulingSystem interface
	virtual void add_action(std::unique_ptr<IBatchAction>&& act) override;
  virtual void flush_actions() override;
  virtual void set_global_schedule_ptr(IGlobalSchedule* gs) override;
  virtual void start_working() override;
  virtual void init() override;
  virtual void stop_working() override;

  // implementing the SchedulerThreadManager interface
	// TODO:
	//    - Make sure that one doesn't wait for the batch too long 
	//    by using a timer. For now we assume high-load within the 
	//    simulations so that timeout never happens.
  SchedulerThreadBatch request_input(SchedulerThread* s) override;
  // In our implementation this overload will merge into global schedule
  // and signal the execution threads.
  void hand_batch_to_execution(
      SchedulerThread* s,
      uint64_t batch_id,
      // TODO: 
      //    Make this a typedef somewhere...
      OrderedWorkload&& workload,
      BatchLockTable&& blt) override;

  virtual ~SchedulerManager();
};

#endif // SCHEDULER_MANAGER_H_
