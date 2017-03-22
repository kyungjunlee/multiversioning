#ifndef EXECUTOR_THREAD_MANAGER_H_
#define EXECUTOR_THREAD_MANAGER_H_

#include "batch/executor_thread.h"

class ExecutorThreadManager {
  public:
    // TODO:
    //    -- Access to global schedule for a particular record?
    virtual void signal_execution_threads(
        std::vector<ExecutorThread::BatchACtions> workload);
};

#endif // EXECUTOR_THREAD_MANAGER_H_
