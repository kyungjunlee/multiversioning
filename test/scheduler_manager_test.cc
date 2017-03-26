#include "gtest/gtest.h"
#include "batch/scheduler_manager.h"
#include "test/test_action.h"
#include "test/test_executor_thread_manager.h"

#include <thread>
#include <memory>

class SchedulerManagerTest :
  public testing::Test,
  public testing::WithParamInterface<int> {
protected:
  std::shared_ptr<SchedulerManager> sm;
  std::shared_ptr<ExecutorThreadManager> etm;
	const uint32_t batch_size = 100;
	const uint32_t batch_length_sec = 0;
	const uint32_t scheduling_threads_count = 3;

  const SchedulingSystemConfig conf = {
		scheduling_threads_count,
		batch_size,
		scheduling_threads_count
	};
	const unsigned int actions_at_start = batch_size * scheduling_threads_count;

	virtual void SetUp() {
    etm = std::make_shared<TestExecutorThreadManager>();
		sm = std::make_shared<SchedulerManager>(this->conf, etm.get());
		// populate the input queue.
		for (unsigned int i = 0;
				i < actions_at_start;
				i++) {
			sm->add_action(
				std::move(
					std::unique_ptr<TestAction>(
						TestAction::make_test_action_with_test_txn({}, {}, i))));
		}
	};
};	

void assertBatchIsCorrect(
		SchedulerThread::BatchActions&& batch,
		unsigned int expected_size,
		unsigned int begin_id,
		unsigned int line) {
	ASSERT_EQ(expected_size, batch.size());
	TestAction* act;
	for (unsigned int i = begin_id; i < begin_id + expected_size; i++) {
		act = static_cast<TestAction*>(batch[i - begin_id].get());
		ASSERT_EQ(i, act->get_id()) <<
			"Error within test starting at line" << line;
	}
}

TEST_F(SchedulerManagerTest, obtain_batchNonConcurrentTest) {
	auto batch = sm->request_input(sm->schedulers[0].get());
	assertBatchIsCorrect(std::move(batch), batch_size, 0, __LINE__);	
	EXPECT_EQ(1, sm->current_input_scheduler);
};

typedef std::function<void (int)> concurrentFun;
void runConcurrentTest(
    concurrentFun fun,
		uint32_t threads_num) {
	std::thread threads[threads_num];
	for (int i = threads_num - 1; i >= 0; i--) {
		threads[i] = std::thread(fun, i);
	}
	
	for (unsigned int i = 0; i < threads_num; i++) threads[i].join();	
}	

concurrentFun get_obtain_batch_test_fun(
    std::shared_ptr<SchedulerManager> sm,
    uint32_t batch_size, 
    int line) {
  return [sm, batch_size, line](int i) {
    auto batch = sm->request_input(sm->schedulers[i].get());
    assertBatchIsCorrect(
        std::move(batch),
        batch_size,
        batch_size * i,
        line);
  };
}

// obtain batch Concurrent No State Overflow Test
//    Does not check whether the state of schedulers not yet waiting
//    is changed.
TEST_P(SchedulerManagerTest, obtain_batchConcurrentNSOFTest) {
  runConcurrentTest(
      get_obtain_batch_test_fun(sm, batch_size, __LINE__),
      scheduling_threads_count - 1);
}

// obtain batch Concurrent State Overflow Test
//    Makes sure that the state of a thead that is not waiting 
//    for input does not get changed preemptively.
TEST_P(SchedulerManagerTest, obtain_batchConcurrentSOFTest) {
  runConcurrentTest(
      get_obtain_batch_test_fun(sm, batch_size, __LINE__),
      scheduling_threads_count);
}

concurrentFun get_signal_exec_threads_test_fun(
    std::shared_ptr<SchedulerManager> sm) {
  return [sm](int i){
    SchedulerManager::OrderedWorkload workload; 
    sm->signal_exec_threads(sm->schedulers[i].get(), std::move(workload)); 
  };  
};

TEST_P(SchedulerManagerTest, signal_execution_threadsConcurrentNSOFTest) {
  runConcurrentTest(
      get_signal_exec_threads_test_fun(sm),
      scheduling_threads_count - 1);
  TestExecutorThreadManager* tetm = 
    static_cast<TestExecutorThreadManager*>(sm->exec_manager);
  ASSERT_EQ(scheduling_threads_count - 1, tetm->signal_execution_threads_called);
};

TEST_P(SchedulerManagerTest, signal_execution_threadsConcurrentSOFTest) {
  runConcurrentTest(
      get_signal_exec_threads_test_fun(sm),
      scheduling_threads_count);
  TestExecutorThreadManager* tetm = 
    static_cast<TestExecutorThreadManager*>(sm->exec_manager);
  ASSERT_EQ(scheduling_threads_count, tetm->signal_execution_threads_called);

};

INSTANTIATE_TEST_CASE_P(
	RerunForEdgeCond,
	SchedulerManagerTest,
	testing::Range(1, 50));
