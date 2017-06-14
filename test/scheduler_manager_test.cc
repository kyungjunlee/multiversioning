#include "gtest/gtest.h"
#include "batch/scheduler_manager.h"
#include "test/test_action.h"
#include "test/test_executor_thread_manager.h"

#include <thread>
#include <memory>
#include <algorithm>

class SchedulerManagerTest :
  public testing::Test,
  public testing::WithParamInterface<int> {
protected:
  std::shared_ptr<SchedulerManager> sm;
  std::shared_ptr<IGlobalSchedule> gs;
  std::shared_ptr<ExecutorThreadManager> etm;
	const uint32_t batch_size = 100;
	const uint32_t batch_length_sec = 0;
	const uint32_t scheduling_threads_count = 3;
  const uint32_t shards = 3;
  const uint32_t first_pin = 0;

  const SchedulingSystemConfig conf = {
		scheduling_threads_count,
		batch_size,
		scheduling_threads_count,
    first_pin,
    shards
	};
  const DBStorageConfig db_conf = {
    .tables_definitions = {{
      .table_id = 0,
      .num_records = 1000
    }}
  }; 
	const unsigned int actions_at_start = batch_size * scheduling_threads_count;

	virtual void SetUp() {
    gs = std::make_shared<GlobalSchedule>();
    etm = std::make_shared<TestExecutorThreadManager>();
		sm = std::make_shared<SchedulerManager>(this->conf, this->db_conf, etm.get());
    sm->gs = gs.get();
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
		std::vector<std::unique_ptr<IBatchAction>>&& batch,
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
  // at this stage we may only assert the existance of the batch since 
  // there are no guarantees made about the order in which batches
  // are assigned to threads.
  ASSERT_EQ(batch_size, batch.batch.size());
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
    std::vector<SchedulerThreadBatch>& batches,
    std::shared_ptr<SchedulerManager> sm, 
    int line) {
  return [&batches, sm, line](int i) {
    batches[i] = sm->request_input(sm->schedulers[i].get());
  };
}

void doObtainBatchTest(
    unsigned int thread_count,
    unsigned int batch_size,
    std::shared_ptr<SchedulerManager> sm,
    bool expect_full_order = true) {
  std::vector<SchedulerThreadBatch> batches(thread_count);

  runConcurrentTest(
      get_obtain_batch_test_fun(batches, sm,  __LINE__),
      thread_count);

  std::sort(batches.begin(), batches.end(), 
      [] (const SchedulerThreadBatch& stb1, const SchedulerThreadBatch& stb2) {
        return stb1.batch_id < stb2.batch_id;
      });

  unsigned int expected_id = 0;
  for (unsigned int i = 0; i < thread_count; i++) {
    if (expect_full_order) {
      expected_id = i;
      ASSERT_EQ(i, batches[i].batch_id);
    } else {
      expected_id = batches[i].batch_id;
    }
    assertBatchIsCorrect(
      std::move(batches[i].batch),
      batch_size,
      expected_id * batch_size,
      __LINE__
    );
  }
};

TEST_P(SchedulerManagerTest, obtain_batchConcurrentTest1) {
  doObtainBatchTest(scheduling_threads_count - 1, batch_size, sm, false);
}

TEST_P(SchedulerManagerTest, obtain_batchConcurrentTest2) {
  doObtainBatchTest(scheduling_threads_count, batch_size, sm);
}

concurrentFun get_signal_exec_threads_test_fun(
    std::shared_ptr<SchedulerManager> sm) {
  return [sm](int i) {
    SchedulerManager::OrderedWorkload workload; 
    BatchLockTable blt;
    sm->hand_batch_to_execution(
        sm->schedulers[i].get(), i, std::move(workload), std::move(blt)); 
  };  
};

TEST_P(SchedulerManagerTest, signal_execution_threadsConcurrentNSOFTest) {
  runConcurrentTest(
      get_signal_exec_threads_test_fun(sm),
      scheduling_threads_count - 2);

  // run another thread to make sure that everything has been pushed through (race cond)
  std::thread g (get_signal_exec_threads_test_fun(sm), scheduling_threads_count - 2);
  g.join();

  TestExecutorThreadManager* tetm = 
    static_cast<TestExecutorThreadManager*>(sm->exec_manager);
  ASSERT_EQ(scheduling_threads_count - 1, tetm->signal_execution_threads_called);
};

TEST_P(SchedulerManagerTest, signal_execution_threadsConcurrentSOFTest) {
  runConcurrentTest(
      get_signal_exec_threads_test_fun(sm),
      scheduling_threads_count - 1);
  // run another thread to make sure that everything has been pushed through (race cond)
  std::thread g (get_signal_exec_threads_test_fun(sm), scheduling_threads_count - 1);
  g.join();

  TestExecutorThreadManager* tetm = 
    static_cast<TestExecutorThreadManager*>(sm->exec_manager);
  ASSERT_EQ(scheduling_threads_count, tetm->signal_execution_threads_called);
};

INSTANTIATE_TEST_CASE_P(
	RerunForEdgeCond,
	SchedulerManagerTest,
	testing::Range(1, 50));
