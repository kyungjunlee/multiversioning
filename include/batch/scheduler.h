#ifndef BATCH_SCHEDULER_H_
#define BATCH_SCHEDULER_H_

#include "batch/batch_action_interface.h"
#include "batch/lock_table.h"
#include "batch/container.h"
#include "batch/scheduler_manager.h"
#include "batch/scheduler_thread.h"

//  Scheduler
//
//    The implementation of a scheduler thread. This is the part of the system
//    that does the actual work of creating batch schedules, merging it within 
//    the global schedule etc.
//
//      TODO:
//          Tests for:
//            - StartWorking 
//            - Init
class Scheduler : public SchedulerThread {
public:
  typedef SchedulerThread::BatchActions BatchActions;
public:
  Scheduler(
      SchedulerThreadManager* manager,
      int m_cpu_number);

  std::unique_ptr<BatchActions> batch_actions;
  BatchLockTable lt;

  void make_batch_schedule();
    // override Runnable interface
  void StartWorking() override;
  void Init() override;
};

#endif // BATCH_SCHEDULER_H_
