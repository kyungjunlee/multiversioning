#ifndef SCHEDULER_THREAD_MANAGER_H_
#define SCHEDULER_THREAD_MANAGER_H_

#include "batch/scheduler_thread.h"

// Scheduler Thread Manager
//
//    This is the general interface between the scheduler thread manager
//    and the scheduling threads. Scheduling threads must communicate with
//    the Scheduler Thread Manager for every action that requires 
//    accessed to shared resources such as the global input queue, 
//    the global schedule and the execution threads input queues.
class SchedulerThreadManager {
  public:
    virtual SchedulerThread::BatchActions request_input(SchedulerThread* s) = 0;
    virtual void signal_exec_threads(SchedulerThread* s) = 0;
    virtual void merge_into_global_schedule(BatchLockTable&& blt) = 0;

    // TODO:
    //    Add a handle to the ExecutionThreadManager.
};

#endif
