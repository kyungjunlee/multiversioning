#include "gtest/gtest.h"
#include "batch/supervisor.h"
#include "test/test_txn.h"
#include "test/db_test_helper.h"
#include "batch/RMW_batch_action.h"

#include <memory>
#include <vector>

class ConsistencyTest : public ::testing::Test {
protected:
  DBTestHelper<Supervisor> hp;

  std::vector<IBatchAction*> getWorkload() {
    unsigned int record_num = 100;
    // prepare the workload
    std::vector<IBatchAction*> workload;
    for (unsigned int i = 0; i < 1000; i++) {
      workload.push_back(new RMWBatchAction(new TestTxn()));
      for (unsigned int j = 0; j < 10; j++) {
        workload[i]->add_write_key({(i + j) % record_num, 0});
        workload[i]->add_read_key({(i + j + 10) % record_num, 0});
      }
    }

    return workload;
  };
};

auto get_assertion() {
  return [](IDBStorage* db) {
    for (unsigned int i = 0; i < 100; i++) {
      ASSERT_EQ(100, db->read_record_value({i, 0}));
    }
  };
}

TEST_F(ConsistencyTest, SingleSchedSingleExec) {
  hp.set_table_info(1, 100)
    .set_exec_thread_num(1)
    .set_sched_thread_num(1)
    .set_batch_size(100)
    .set_workload(std::move(getWorkload()));

  hp.runTest(get_assertion());
}

TEST_F(ConsistencyTest, SingleSchedTwoExec) {
  hp.set_table_info(1, 100)
    .set_exec_thread_num(2)
    .set_sched_thread_num(1)
    .set_batch_size(100)
    .set_workload(std::move(getWorkload()));

  hp.runTest(get_assertion());
}

TEST_F(ConsistencyTest, TwoSchedTwoExec) {
  hp.set_table_info(1, 100)
    .set_exec_thread_num(2)
    .set_sched_thread_num(2)
    .set_batch_size(100)
    .set_workload(std::move(getWorkload()));

  hp.runTest(get_assertion());
}
