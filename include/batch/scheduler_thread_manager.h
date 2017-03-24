#ifndef SCHEDULER_THREAD_MANAGER_H_
#define SCHEDULER_THREAD_MANAGER_H_

#include "batch/scheduler_thread.h"
#include "batch/executor_thread_manager.h"

// Scheduler Thread Manager
//
//    This is the general interface between the scheduler thread manager
//    and the scheduling threads. Scheduling threads must communicate with
//    the Scheduler Thread Manager for every action that requires 
//    accessed to shared resources such as the global input queue, 
//    the global schedule and the execution threads input queues.
class SchedulerThreadManager {
  public:
    ExecutorThreadManager* exec_manager;
    /*
     * An ordered workload is a batch workload that is "ordered" in the sense
     * that the less likely to collide transactions are first. Think of this
     * as a flattenned out hierarchy of packings (appended packings);
     */
    typedef std::vector<std::shared_ptr<IBatchAction>> OrderedWorkload;

    SchedulerThreadManager(ExecutorThreadManager* exec): exec_manager(exec) {};
    virtual SchedulerThread::BatchActions request_input(SchedulerThread* s) = 0;
    virtual void signal_exec_threads(
        SchedulerThread* s,
        OrderedWorkload&& workload) = 0;
    virtual void merge_into_global_schedule(
        SchedulerThread* s,
        BatchLockTable&& blt) = 0;
};

#endif
