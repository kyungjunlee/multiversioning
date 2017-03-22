#ifndef TEST_EXECUTOR_THREAD_MANAGER_H_
#define TEST_EXECUTOR_THREAD_MANAGER_H_

class TestExecutorThreadManager : public ExecutorThreadManager {
public:
  unsigned int signal_execution_threads_called = 0;

  void signal_execution_threads(
      std::vector<ExecutorThread::BatchActions> workload) {
    (void) workload;
    signal_execution_threads_called ++;
  }

  virtual ~TestExecutorThreadManager();
};

#endif // TEST_EXECUTOR_THREAD_MANAGER_H_
