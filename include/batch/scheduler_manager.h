#ifndef SCHEDULER_MANAGER_H_
#define SCHEDULER_MANAGER_H_

#include "batch/scheduler.h"
#include "batch/lock_table.h"
#include "batch/input_queue.h"
#include "batch/scheduler_system.h"
#include "batch/scheduler_thread_manager.h"

#include <vector>
#include <memory>

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
  bool inputs_are_ok(
      SchedulerThread *s,
      std::vector<SchedulerState> possible_stated_of_s);
  bool system_is_initialized();
public:
  uint64_t current_input_scheduler;
  uint64_t current_signaling_scheduler;
	uint64_t current_merging_scheduler;
  
  std::unique_ptr<InputQueue> iq;
	std::vector<std::shared_ptr<SchedulerThread>> schedulers;
  SchedulerManager(
      SchedulingSystemConfig c,
      ExecutorThreadManager* exec);

  // implementing the SchedulingSystem interface
	void add_action(std::unique_ptr<BatchAction>&& act) override;
  void create_threads() override;
  void start_working() override;
  void init_threads() override;

  // implementing the SchedulerThreadManager interface
	// TODO:
	//    - Make sure that one doesn't wait for the batch too long 
	//    by using a timer. For now we assume high-load within the 
	//    simulations so that timeout never happens.
	//    - Profile this to see whether we need "batch dequeues" to
	//    expedite the process of obtaining a batch.
  SchedulerThread::BatchActions request_input(SchedulerThread* s) override;
  virtual void signal_exec_threads(
      SchedulerThread* s,
      ExecutorThreadManager::SignalWorkload&& workload) override;
	void merge_into_global_schedule(
      SchedulerThread* s,
      BatchLockTable&& blt) override;
};

#endif // SCHEDULER_MANAGER_H_
