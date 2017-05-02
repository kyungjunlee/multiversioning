#ifndef TEST_SCHEDULER_THREAD_MANAGER_H_
#define TEST_SCHEDULER_THREAD_MANAGER_H_

#include "batch/scheduler_thread_manager.h"

class TestSchedulerThreadManager : public SchedulerThreadManager {
public:
  TestSchedulerThreadManager(ExecutorThreadManager* exec):
    SchedulerThreadManager(exec)
  {};

  uint64_t request_input_called = 0;
  uint64_t hand_batch_called = 0;

  SchedulerThreadBatch request_input(SchedulerThread* s) override {
    (void) s;
    request_input_called ++;
    return SchedulerThreadBatch{};
  };

  void hand_batch_to_execution(
      SchedulerThread* d,
      uint64_t batch_id,
      OrderedWorkload&& workload,
      BatchLockTable&& blt) override {
    (void) d;
    (void) batch_id;
    (void) workload;
    (void) blt;
    hand_batch_called ++;
  };
};

#endif // TEST_SCHEDULER_THREAD_MANAGER_H_
