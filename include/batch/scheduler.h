#ifndef BATCH_SCHEDULER_H_
#define BATCH_SCHEDULER_H_

#include "batch/batch_action.h"
#include "batch/lock_table.h"
#include "batch/MS_queue.h"
#include "batch/container.h"
#include "batch/input_queue.h"
#include "runnable.hh"

class InputQueue;

// TODO:
//    - Add handle to the global schedule here so that we may merge 
//      the local schedule in every time we need to. 
struct SchedulerConfig {
  uint32_t batch_size_act;
  uint32_t batch_length_sec;
  int m_cpu_number;
  InputQueue* input;
  // TODO:
  //   GlobalSchedule* gc;
};

enum class SchedulerState { 
  waiting_for_input = 0,
  input,
  batch_creation,
  waiting_to_merge,
  batch_merging,
  waiting_to_signal_execution,
  signaling_execution,
  state_count
};

// TODO:
//    - Resolve the issue of a lifecycle of a transaction. Should we have a
//      single producer/single consumer input queue with a counter whose
//      mod # of scheduling threads dictates which thread is handling a batch?
//      Should we have a direct way of enqueuing to a given scheduling thread?
//      The latter seems like a cheat, the former might be a bottle neck, but perhaps
//      not so much.
//
//      For now I will go with a global input queue implemented in "batch/input_queue.h".
//      Threads will pass the lock on the input queue among themselves using CAS so that
//      not much contention is to be expected.
//
//    State transition of scheduler:
//      - waiting_for_input 
//      - input 
//      - batch_creation 
//      - waiting to merge 
//      - batch merging
//      - waiting to signal execution
//      - signaling execution
//      - waiting for input
//      - ...
//
//
//      TODO:
//          Tests for:
//            - StartWorking 
//            - Init
//            - make_batch_schedule
class Scheduler : public Runnable {
public:
  typedef Container::BatchActions BatchActions;
protected:
  std::unique_ptr<BatchActions> batch_actions;
  BatchLockTable lt;
  SchedulerConfig conf;
  SchedulerState state;

  // state change is done using CAS and is thread safe.
  bool change_state(SchedulerState nextState, SchedulerState expectedCurrState);
  void make_batch_schedule();
public:
  Scheduler(SchedulerConfig sc);

  // override Runnable interface
  void StartWorking() override;
  void Init() override;

  // own functions
  SchedulerState get_state();
  unsigned int get_max_actions();

  virtual void put_action(std::unique_ptr<BatchAction> act);

  // All of the below are thread safe.
  bool signal_waiting_for_input();
  bool signal_input();
  bool signal_batch_creation();
  bool signal_waiting_for_merge();
  bool signal_merging();
  bool signal_waiting_for_exec_signal();
  bool signal_exec_signal();
};

#endif // BATCH_SCHEDULER_H_