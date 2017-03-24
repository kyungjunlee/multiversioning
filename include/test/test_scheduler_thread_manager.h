#ifndef TEST_SCHEDULER_THREAD_MANAGER_H_
#define TEST_SCHEDULER_THREAD_MANAGER_H_

#include "batch/scheduler_thread_manager.h"

class TestSchedulerThreadManager : public SchedulerThreadManager {
public:
  TestSchedulerThreadManager(ExecutorThreadManager* exec):
    SchedulerThreadManager(exec)
  {};

  uint64_t request_input_called = 0;
  uint64_t signal_exec_threads_called = 0;
  uint64_t merge_into_global_schedule_called = 0;

  SchedulerThread::BatchActions request_input(SchedulerThread* s) override {
    (void) s;
    request_input_called ++;
    return SchedulerThread::BatchActions();
  };

  void signal_exec_threads(
      SchedulerThread* s,
      OrderedWorkload&& workload) override {
    (void) s;
    (void) workload;
    signal_exec_threads_called ++;
  };

  void merge_into_global_schedule(
      SchedulerThread* s,
      BatchLockTable&& blt) override {
    (void) s;
    (void) blt;
    merge_into_global_schedule_called ++;
  };

};

#endif // TEST_SCHEDULER_THREAD_MANAGER_H_
