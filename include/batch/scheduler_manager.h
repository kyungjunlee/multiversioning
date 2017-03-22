#ifndef SCHEDULER_MANAGER_H_
#define SCHEDULER_MANAGER_H_

#include "batch/scheduler.h"
#include "batch/lock_table.h"
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
protected:
  // imlementation of the protected part of SchedulingSystem interface.
	void add_action(std::unique_ptr<BatchAction>&& act) override;

public:
  uint64_t current_input_scheduler;
  uint64_t current_signaling_scheduler;
	uint64_t current_merging_scheduler;
  
	std::vector<std::shared_ptr<SchedulerThread>> schedulers;
  SchedulerManager(SchedulingSystemConfig c);

  // implementing the SchedulingSystem interface
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
  void signal_exec_threads(SchedulerThread* s) override;
	void merge_into_global_schedule(BatchLockTable&& blt) override;
};

#endif // SCHEDULER_MANAGER_H_
