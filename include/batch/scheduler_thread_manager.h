#ifndef SCHEDULER_THREAD_MANAGER_H_
#define SCHEDULER_THREAD_MANAGER_H_

#include "batch/scheduler_thread.h"

class SchedulerThreadManager {
  public:
    virtual SchedulerThread::BatchActions request_input(SchedulerThread* s) = 0;
    virtual void signal_exec_threads(SchedulerThread* s) = 0;
    virtual void merge_into_global_schedule(BatchLockTable&& blt) = 0;
};

#endif
