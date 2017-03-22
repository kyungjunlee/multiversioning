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
    
    SchedulerThread(
        SchedulerThreadManager* manager,
        int m_cpu_number): 
      Runnable(m_cpu_number),
      manager(manager)
    {};

  public:
    using Runnable::StartWorking;
    using Runnable::Init;

    typedef Container::BatchActions BatchActions;
};

#endif // SCHEDULER_THREAD_H_
