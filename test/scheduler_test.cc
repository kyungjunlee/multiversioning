#include "gtest/gtest.h"
#include "batch/scheduler.h"
#include "batch/input_queue.h"
#include "test/test_executor_thread_manager.h"
#include "test/test_scheduler_thread_manager.h"
#include "test/test_action.h"

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
  std::shared_ptr<SchedulerThreadManager> stm;

  virtual void SetUp() {
    etm = std::make_shared<TestExecutorThreadManager>();
    stm = std::make_shared<TestSchedulerThreadManager>(etm.get());
    s = std::make_shared<Scheduler>(stm.get(), 0);    
  };

  virtual void add_test_txn_to_scheduler_batch(
      IBatchAction::RecordKeySet write_set,
      IBatchAction::RecordKeySet read_set,
      uint64_t id) {
    // allocate the object if that didn't happen yet.
    if (s->batch_actions == nullptr) {
      s->batch_actions = std::make_unique<Scheduler::BatchActions>();
    }

    s->batch_actions->push_back(
        std::move(
          std::make_unique<TestAction>(
            new TestTxn(), write_set, read_set, id)));
  }; 
};

// Parsing the following typedefs:
//    We want to input a vector of pairs
//     {Record Key, std::vector<sets of txn ids within stages>}
typedef std::unordered_set<uint64_t> ExpectedTxnIds;
typedef std::vector<ExpectedTxnIds> ExpectedStages;
typedef std::pair<RecordKey, ExpectedStages> ExpectedRecordQueue;
typedef std::vector<ExpectedRecordQueue> ExpectedLockTable;
void assert_correct_schedule(
    std::shared_ptr<Scheduler> s,
    ExpectedLockTable e) {
  auto collect_ids_from_lock_stage = [](std::shared_ptr<LockStage> ls){
    const LockStage::RequestingActions& r = ls->get_requesters();
    std::unordered_set<uint64_t> ids;

    for (const auto& e : r) {
      TestAction* ta = static_cast<TestAction*>(e.get());
      ids.insert(ta->get_id());
    }

    return ids;
  };

  // batch lock table
  const auto& blt = s->lt.get_lock_table_data();

  for (auto rec_pair : e) {
    // batch lock queue, must exist.
    auto blt_elt = blt.find(rec_pair.first);
    ASSERT_TRUE(blt_elt != blt.end());
    auto blq = blt_elt->second; 

    // head of the queue musn't be null.
    auto curr_blq_elt = blq->peek_head_elt();

    // transaction in each stage must correspond to what we expect.
    for (auto stage_ids : rec_pair.second) {
      ASSERT_TRUE(curr_blq_elt != nullptr);
      auto curr_lock_stage_ptr = curr_blq_elt->get_contents();
      auto ids_in_lock_stage = collect_ids_from_lock_stage(*curr_lock_stage_ptr); 
      ASSERT_EQ(stage_ids.size(), ids_in_lock_stage.size());
      ASSERT_EQ(stage_ids, ids_in_lock_stage); 

      curr_blq_elt = curr_blq_elt->get_next_elt();
    }

    // there are no superfluous elements in the blq.
    ASSERT_TRUE(curr_blq_elt == nullptr);
  }
};

// Input, all exclusive:
//    T0: 1
//    T1: 1, 3, 4
//    T2: 2, 3
//
// Expected Output:
//  1 <- T0 <- T1
//  2 <- T2
//  3 <- T2 <- T1
//  4 <- T1
TEST_F(SchedulerTest, process_batch_Test1) {
  add_test_txn_to_scheduler_batch({1}, {}, 0);
  add_test_txn_to_scheduler_batch({1, 3, 4}, {}, 1);
  add_test_txn_to_scheduler_batch({2, 3}, {}, 2);
  s->process_batch();

  assert_correct_schedule(
      s,
      { {1, {{0}, {1}}},
        {2, {{2}}},
        {3, {{2}, {1}}},
        {4, {{1}}}});
}

// Input, all exclusive:
//    T0: 1
//    T1: 2, 3, 4, 5
//    T2: 1, 2, 4
//
// Expected Output:
//  1 <- T0 <- T2
//  2 <- T1 <- T2
//  3 <- T1
//  4 <- T1 <- T2
//  5 <- T1
TEST_F(SchedulerTest, process_batch_Test2) {
  add_test_txn_to_scheduler_batch({1}, {}, 0);
  add_test_txn_to_scheduler_batch({2, 3, 4, 5}, {}, 1);
  add_test_txn_to_scheduler_batch({1, 2, 4}, {}, 2);
  s->process_batch();

  assert_correct_schedule(
     s,
     { {1, {{0}, {2}}},
       {2, {{1}, {2}}},
       {3, {{1}}},
       {4, {{1}, {2}}},
       {5, {{1}}}}); 
};

// Input, all shared:
//    T0: 1, 3
//    T1: 1, 2
//    T2: 2, 3
// Expected Output:
//  1 <- T0, T1
//  2 <- T1, T2
//  3 <- T0, T2
TEST_F(SchedulerTest, process_batchr_Test3) {
  add_test_txn_to_scheduler_batch({}, {1, 3}, 0);
  add_test_txn_to_scheduler_batch({}, {1, 2}, 1);
  add_test_txn_to_scheduler_batch({}, {2, 3}, 2);
  s->process_batch();

  assert_correct_schedule(
      s,
      { {1, {{0, 1}}},
        {2, {{1, 2}}},
        {3, {{0, 2}}}});
} 

// Input, mixed:
//  T0: 1s, 2
//  T1: 1s, 3
//  T2: 1s, 2s, 3s
// Expected Output:
//  1 <- T0, T1, T2
//  2 <- T0 <- T2
//  3 <- T1 <- T2
TEST_F(SchedulerTest, process_batchr_Test4) {
  add_test_txn_to_scheduler_batch({2}, {1}, 0);
  add_test_txn_to_scheduler_batch({3}, {1}, 1);
  add_test_txn_to_scheduler_batch({}, {1, 2, 3}, 2);
  s->process_batch();

  assert_correct_schedule(
      s,
      { {1, {{0, 1, 2}}},
        {2, {{0}, {2}}},
        {3, {{1}, {2}}}});
};

// TODO: More packing tests and benchmarking
