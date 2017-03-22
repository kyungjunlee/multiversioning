#ifndef BATCH_SCHEDULER_H_
#define BATCH_SCHEDULER_H_

#include "batch/batch_action.h"
#include "batch/lock_table.h"
#include "batch/container.h"
#include "batch/scheduler_manager.h"
#include "batch/scheduler_thread.h"

//  Scheduler
//
//
//    The implementation of a scheduler thread. This is the part of the system
//    that does the actual work of creating batch schedules, merging it within 
//    the global schedule etc.
//
//      TODO:
//          Tests for:
//            - StartWorking 
//            - Init
//            - make_batch_schedule
class Scheduler : public SchedulerThread {
public:
  typedef SchedulerThread::BatchActions BatchActions;
protected:
  std::unique_ptr<BatchActions> batch_actions;
  BatchLockTable lt;

  // state change is done using CAS and is thread safe.
  bool change_state(SchedulerState nextState, SchedulerState expectedCurrState);
  void make_batch_schedule();
public:
  Scheduler(
      SchedulerThreadManager* manager,
      int m_cpu_number);

  // override Runnable interface
  void StartWorking() override;
  void Init() override;

  // implement the scheduler thread interface.
  SchedulerState get_state() override;
  bool signal_waiting_for_input() override;
  bool signal_input() override;
  bool signal_batch_creation() override;
  bool signal_waiting_for_merge() override;
  bool signal_merging() override;
  bool signal_waiting_for_exec_signal() override;
  bool signal_exec_signal() override;
};

#endif // BATCH_SCHEDULER_H_
