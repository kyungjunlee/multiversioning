#ifndef EXECUTOR_THREAD_MANAGER_H_
#define EXECUTOR_THREAD_MANAGER_H_

#include "batch/executor_thread.h"

class ExecutorThreadManager {
  public:
    // TODO:
    //    -- Access to global schedule for a particular record?
    typedef std::vector<ExecutorThread::BatchActions> SignalWorkload;
    virtual void signal_execution_threads(SignalWorkload&& workload);

    virtual ~ExecutorThreadManager();
};

#endif // EXECUTOR_THREAD_MANAGER_H_
