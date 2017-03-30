#include "gtest/gtest.h"
#include "batch/supervisor.h"
#include "test/test_txn.h"
#include "test/db_test_helper.h"
#include "batch/RMW_batch_action.h"

#include <memory>
#include <vector>

std::vector<std::unique_ptr<IBatchAction>> getWorkload() {
  unsigned int record_num = 100;
  // prepare the workload
  std::vector<std::unique_ptr<IBatchAction>> workload;
  for (unsigned int i = 0; i < 1000; i++) {
    workload.push_back(std::make_unique<RMWBatchAction>(new TestTxn()));
    for (unsigned int j = 0; j < 10; j++) {
      workload[i]->add_write_key({(i + j) % record_num, 0});
      workload[i]->add_read_key({(i + j + 10) % record_num, 0});
    }
  }

  return workload;
};

auto get_assertion() {
  return [](IDBStorage* db) {
    for (unsigned int i = 0; i < 100; i++) {
      ASSERT_EQ(100, db->read_record_value({i, 0}));
    }
  };
}

TEST(ConsistencyTest, SingleSchedSingleExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 100)
    .set_exec_thread_num(1)
    .set_sched_thread_num(1)
    .set_batch_size(100)
    .set_workload(std::move(getWorkload()));

  hp.runTest(get_assertion());
}

TEST(ConsistencyTest, SingleSchedTwoExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 100)
    .set_exec_thread_num(2)
    .set_sched_thread_num(1)
    .set_batch_size(100)
    .set_workload(std::move(getWorkload()));

  hp.runTest(get_assertion());
}

TEST(ConsistencyTest, TwoSchedTwoExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 100)
    .set_exec_thread_num(2)
    .set_sched_thread_num(2)
    .set_batch_size(100)
    .set_workload(std::move(getWorkload()));

  hp.runTest(get_assertion());
}
