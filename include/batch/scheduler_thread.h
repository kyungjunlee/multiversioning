#ifndef SCHEDULER_THREAD_H_
#define SCHEDULER_THREAD_H_

#include "runnable.hh"

//  Scheduler Thread
//
//
//      The general interface of a scheduler thread required by the 
//      Scheduler Thread Manager. 
class SchedulerThreadManager;
class SchedulerThread : public Runnable {
  protected:
    SchedulerThreadManager* manager;
    uint64_t stop_signal;
    
    SchedulerThread(
        SchedulerThreadManager* manager,
        int m_cpu_number): 
      Runnable(m_cpu_number),
      manager(manager),
      stop_signal(false)
    {};

  public:
    using Runnable::StartWorking;
    using Runnable::Init;

    typedef Container::BatchActions BatchActions;
    virtual void signal_stop_working() = 0;
    virtual bool is_stop_requested() = 0;
 
    virtual ~SchedulerThread() {
      free(m_rand_state); 
    };
};

#endif // SCHEDULER_THREAD_H_
