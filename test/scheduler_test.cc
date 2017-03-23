#include "gtest/gtest.h"
#include "batch/scheduler.h"
#include "test/test_executor_thread_manager.h"
#include "test/test_scheduler_thread_manager.h"

#include <memory>
#include <utility>
#include <vector>

class SchedulerTest : public testing::Test {
protected:
  const uint32_t batch_size = 100;
  // this parameter does not matter for now
  const uint32_t batch_length = 0;
  // this parameter does not matter for the test
  const int m_cpu_num = 0;
  std::shared_ptr<InputQueue> iq;
  std::shared_ptr<Scheduler> s;
  std::shared_ptr<ExecutorThreadManager> etm;

  virtual void SetUp() {
    etm = std::make_shared<TestExecutorThreadManager>();
    s = std::make_shared<Scheduler>(
      new TestSchedulerThreadManager(etm.get()), 0);    
  };
};
