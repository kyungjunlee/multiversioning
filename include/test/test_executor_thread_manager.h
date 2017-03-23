#ifndef TEST_EXECUTOR_THREAD_MANAGER_H_
#define TEST_EXECUTOR_THREAD_MANAGER_H_

#include "batch/executor_thread_manager.h"

class TestExecutorThreadManager : public ExecutorThreadManager {
public:
  unsigned int signal_execution_threads_called = 0;

  void signal_execution_threads(
      ExecutorThreadManager::SignalWorkload&& workload) override {
    (void) workload;
    signal_execution_threads_called ++;
  }
};

#endif // TEST_EXECUTOR_THREAD_MANAGER_H_
