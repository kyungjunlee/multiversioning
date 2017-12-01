#ifndef BATCH_SCHEDULER_HELPER_H_
#define BATCH_SCHEDULER_HELPER_H_

#include "batch/batch_action_interface.h"
#include "batch/lock_table.h"
#include "batch/container.h"
#include "batch/scheduler_thread_manager.h"
#include "batch/scheduler_thread.h"
#include "batch/diagnostics.h"

//  SchedulerHelper
//
//    The implementation of a scheduler thread. This is the part of the system
//    that does the actual work of merging it within the global schedule and
//    assigning ready packings to executing threads.
//
//      TODO:
//          Tests for:
//            - StartWorking 
//            - Init
class SchedulerHelper : public SchedulerThread {
private:
  uint64_t thread_id;
public:
  Scheduler(
      SchedulerThreadManager* manager,
      int m_cpu_number,
      uint64_t thread_id);

  /*
  SchedulerThreadBatch batch_actions;
  BatchLockTable lt;
  SchedulerThreadManager::OrderedWorkload workloads;
  */
  
  // override SchedulerThread interface
  void signal_stop_working() override;
  bool is_stop_requested() override;
  void reset() override;

  // override Runnable interface
  void StartWorking() override;
  void Init() override;

  virtual ~Scheduler();

  IF_SCHED_DIAG(
    SchedulerDiag diag;
  );
};

#endif // BATCH_SCHEDULER_HELPER_H_
