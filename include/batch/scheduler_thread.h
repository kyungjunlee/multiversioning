#ifndef SCHEDULER_THREAD_H_
#define SCHEDULER_THREAD_H_

#include "runnable.hh"

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
//      The interface that is required by the SchedulerManager to communicate
//      with Scheduler threads properly. We require scheduling threads to:
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
