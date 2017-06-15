#ifndef SCHEDULER_MANAGER_H_
#define SCHEDULER_MANAGER_H_

#include "batch/scheduler.h"
#include "batch/lock_table.h"
#include "batch/batched_input_queue.h"
#include "batch/scheduler_system.h"
#include "batch/scheduler_thread_manager.h"
#include "batch/db_storage_interface.h"

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

// Thread Input Queues
//
// Every scheduler worker has its own input queue. The queue is populated
// by a scheduler worker which has obtained the proper lock. Thus, every
// worker obtains input from its own queue with little contention on 
// global objects.
class ThreadInputQueues {
private: 
  pthread_rwlock_t input_lock;
  uint64_t input_batch_id;
  std::vector<ThreadInputQueue> queues;
  BatchedInputQueue iq;

public:
  ThreadInputQueues(
      unsigned int thread_number,
      unsigned int batch_size_act);
  
  void add_action(std::unique_ptr<IBatchAction>&& act);
  void flush_actions();
  bool unassigned_input_exists();

  bool input_awaits(SchedulerThread* s);
  ThreadInputQueue& get_my_queue(SchedulerThread* s);
  SchedulerThreadBatch get_batch_from_my_queue(SchedulerThread* s);
  void assign_inputs(SchedulerThread* s);
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
class AwaitingBatchQueue : public MSQueue<AwaitingBatch> {
  private:
    using MSQueue<AwaitingBatch>::merge_queue;
};

// Awaiting Scheduler Batches
//
//  Every scheduler worker has its own queue for batches whose schedules
//  have been created. These queues are joined into a global, ordered
//  vector of batches later on. Every queue in question is single-consumer,
//  single-producer.
struct AwaitingSchedulerBatches {
  AwaitingSchedulerBatches(unsigned int thread_number) {
    pending_queues.resize(thread_number);
  };

  std::vector<AwaitingBatchQueue> pending_queues;
};

// Global Ordered Batches
//
//  This is the object representing the global, ordered batches (ordered
//  on batch ID). Also includes the necessary locks and counters. As per
//  usual, the lock must be obtained before any operation on the data
//  is performed.
struct GlobalOrderedBatches {
  GlobalOrderedBatches():
    handed_batch_id(0)
  {
    pthread_rwlock_init(&lck, NULL);
  };
  
  std::vector<AwaitingBatch> batches;
  pthread_rwlock_t lck;
  uint64_t handed_batch_id;
};

// Batch Merging Stage Queues
//
//  Merging of batch schedules into the global schedule is sharded horizontally
//  and so it is done in stages. The queues below are the input and output 
//  queues for each stage. The additional queue is required for signaling
//  to the execution threads that new actions are to be executed. Signaling
//  is treated as the last stage of merging.
struct BatchMergingStageQueues {
  BatchMergingStageQueues(unsigned int stages_number) {
    merging_stages.resize(stages_number);
    stage_locks.resize(stages_number);
    for (auto& lck : stage_locks) {
      pthread_rwlock_init(&lck, NULL);
    }
    pthread_rwlock_init(&signaling_lock, NULL);
  };

  std::vector<AwaitingBatchQueue> merging_stages;
  std::vector<pthread_rwlock_t> stage_locks;
  AwaitingBatchQueue merged_batches;
  pthread_rwlock_t signaling_lock;
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

  // convert the given workload batch from the scheduler worker format
  // to the format accepted by the execution system.
  ExecutorThreadManager::ThreadWorkloads convert_to_exec_format(
      OrderedWorkload&& ow);

public:
  IF_SCHED_MAN_DIAG(
    SchedulerManagerDiag diag
  );

  // Variables necessary for input
  ThreadInputQueues thread_input;

  // Local schedule queues.
  AwaitingSchedulerBatches pending_batches;

  // Synchronized global vector of batches waiting to be passed to 
  // the execution. Sorted on batch id.  
  GlobalOrderedBatches sorted_pending_batches;

  // Variables necessary for horizontally shareded merging into the 
  // global schedule.
  const unsigned int records_per_stage;
  BatchMergingStageQueues merging_queues;
  
  std::vector<std::shared_ptr<SchedulerThread>> schedulers;
  IGlobalSchedule* gs;

  SchedulerManager(
      SchedulingSystemConfig c,
      DBStorageConfig db_c,
      ExecutorThreadManager* exec);

  // implementing the SchedulingSystem interface
	virtual void add_action(std::unique_ptr<IBatchAction>&& act) override;
  virtual void flush_actions() override;
  virtual void set_global_schedule_ptr(IGlobalSchedule* gs) override;
  virtual void start_working() override;
  virtual void init() override;
  virtual void reset() override;
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

  void register_created_batch(
      SchedulerThread* s,
      uint64_t batch_id,
      OrderedWorkload&& workload,
      BatchLockTable&& blt);
  void process_created_batches();
  void collect_awaiting_batches();
  void merge_into_global_schedule(unsigned int stage);
  void signal_execution_threads(); 

  virtual ~SchedulerManager();
};

#endif // SCHEDULER_MANAGER_H_
