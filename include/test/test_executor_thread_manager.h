#ifndef TEST_EXECUTOR_THREAD_MANAGER_H_
#define TEST_EXECUTOR_THREAD_MANAGER_H_

#include "batch/executor_thread_manager.h"

class TestExecutorThreadManager : public ExecutorThreadManager {
public:
  unsigned int signal_execution_threads_called = 0;
  unsigned int finalize_action_called = 0;
  unsigned int get_current_lock_holder_for_called = 0;

  void signal_execution_threads(
      ExecutorThreadManager::SignalWorkload&& workload) override {
    (void) workload;
    signal_execution_threads_called ++;
  };

  std::shared_ptr<LockStage> get_current_lock_holder_for(RecordKey key) override {
    (void) key;
    get_current_lock_holder_for_called ++;
    return nullptr;
  };

  void finalize_action(std::shared_ptr<BatchActionInterface> act) override {
    (void) act;
    finalize_action_called ++;
  };
};

#endif // TEST_EXECUTOR_THREAD_MANAGER_H_
