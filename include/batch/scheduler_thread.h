#ifndef SCHEDULER_THREAD_H_
#define SCHEDULER_THREAD_H_

#include "runnable.hh"

// Scheduler State
//  
//    Scheduler Threads may be thoughts of as state machines with the state
//    dictating what the thread may and may not do. This state is used by the 
//    Scheduler Thread Manager to determine whether or not a thread is ready
//    to perform a given action without resolving to usage of shared memory.
//
//    See include/batch/scheduler_manager.h and its implementation for more
//    information. 
//
//    Note:
//        The state machine proceeds through the states one by one and never
//        skips states.
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

//  Scheduler Thread
//
//
//      The general interface of a scheduler thread required by the 
//      Scheduler Thread Manager. 
class SchedulerThreadManager;
class SchedulerThread : public Runnable {
  protected:
    SchedulerState state;
    SchedulerThreadManager* manager;
    
    SchedulerThread(
        SchedulerThreadManager* manager,
        int m_cpu_number): 
      Runnable(m_cpu_number),
      state(SchedulerState::waiting_for_input),
      manager(manager)
    {};

  public:
    using Runnable::StartWorking;
    using Runnable::Init;

    typedef Container::BatchActions BatchActions;

    virtual SchedulerState get_state() = 0;
    virtual bool signal_waiting_for_input() = 0;
    virtual bool signal_input() = 0;
    virtual bool signal_batch_creation() = 0;
    virtual bool signal_waiting_for_merge() = 0;
    virtual bool signal_merging() = 0;
    virtual bool signal_waiting_for_exec_signal() = 0;
    virtual bool signal_exec_signal() = 0;
};

#endif // SCHEDULER_THREAD_H_
