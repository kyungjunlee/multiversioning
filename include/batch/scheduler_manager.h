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
  bool system_is_initialized();
  void create_threads();
public:
  uint64_t current_input_scheduler;
  uint64_t current_signaling_scheduler;
	uint64_t current_merging_scheduler;
  
  std::unique_ptr<InputQueue> iq;
	std::vector<std::shared_ptr<SchedulerThread>> schedulers;
  IGlobalSchedule* gs;

  SchedulerManager(
      SchedulingSystemConfig c,
      ExecutorThreadManager* exec);

  // implementing the SchedulingSystem interface
	virtual void add_action(std::unique_ptr<IBatchAction>&& act) override;
  virtual void set_global_schedule_ptr(IGlobalSchedule* gs) override;
  virtual void start_working() override;
  virtual void init() override;

  // implementing the SchedulerThreadManager interface
	// TODO:
	//    - Make sure that one doesn't wait for the batch too long 
	//    by using a timer. For now we assume high-load within the 
	//    simulations so that timeout never happens.
  SchedulerThread::BatchActions request_input(SchedulerThread* s) override;
  virtual void signal_exec_threads(
      SchedulerThread* s,
      OrderedWorkload&& workload) override;
  // TODO: tests for this
	virtual void merge_into_global_schedule(
      SchedulerThread* s,
      BatchLockTable&& blt) override;
};

#endif // SCHEDULER_MANAGER_H_
