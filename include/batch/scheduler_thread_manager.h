#ifndef SCHEDULER_THREAD_MANAGER_H_
#define SCHEDULER_THREAD_MANAGER_H_

#include "batch/batch_action_interface.h"
#include "batch/scheduler_thread.h"
#include "batch/executor_thread_manager.h"

#include <vector>
#include <memory>

// Scheduler Thread Manager
//
//    This is the general interface between the scheduler thread manager
//    and the scheduling threads. Scheduling threads must communicate with
//    the Scheduler Thread Manager for every action that requires 
//    accessed to shared resources such as the global input queue, 
//    the global schedule and the execution threads input queues.
class SchedulerThreadManager {
  public:
    typedef std::vector<IBatchAction*> OrderedWorkload;
    ExecutorThreadManager* exec_manager;

    SchedulerThreadManager(ExecutorThreadManager* exec): exec_manager(exec) {};
    virtual SchedulerThreadBatch request_input(SchedulerThread* s) = 0;
    // The workload below should be ordered in the sense that the less likely
    // to collide transactions should be present in the front of the workload.
    // You may think of this requirement as a flattenned out hierarchical packing
    // (appended to one another).
    virtual void hand_batch_to_execution(
        SchedulerThread* s,
        uint64_t batch_id,
        OrderedWorkload&& workload,
        BatchLockTable&& blt) = 0;
    
    virtual ~SchedulerThreadManager(){};
};

#endif
