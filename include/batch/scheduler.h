#ifndef BATCH_SCHEDULER_H_
#define BATCH_SCHEDULER_H_

#include "batch/batch_action.h"
#include "batch/lock_table.h"
#include "batch/container.h"
#include "batch/scheduler_manager.h"
#include "batch/scheduler_thread.h"

//  Scheduler
//
//    Implementation of SchedulingThread. Scheduling threads may be 
//    thought of as state machines. The state transitions are described below.
//
//    State transition of scheduler:
//      - waiting_for_input 
//          -- Awaits its turn to dequeue actions from the input queue. Input
//            Queue is a single-producer single-consumer MS-queue that contains 
//            all of the transactions in the system.
//      - input 
//          -- Dequeues transactions from the input queue.
//      - batch_creation 
//          -- Creates the batch schedule. This is done offline and does not
//          conflict with anything else in the system.
//      - waiting to merge 
//          -- Awaits for the logical lock on merging its schedule into the 
//          global schedule.
//      - batch merging
//          -- Merges into the global schedule.
//      - waiting to signal execution
//          -- Awaits for the logical lock on signaling to execution threads.
//          Signaling execution threads means passing them pointers to
//          actions that they own. For more information see include/batch/executor.h
//      - signaling execution
//          -- Signals execution threads
//      - waiting for input
//      - ...
//
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

  // implement the scheduler interface.
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
