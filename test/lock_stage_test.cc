#include "gtest/gtest.h"
#include "batch/lock_stage.h"
#include "test/test_lock_stage.h"
#include "test/test_action.h"

#include <thread>

class LockStageTest : public testing::Test {
protected:
  void expect_holders_to_be(LockStage& ls, uint64_t n) {
    ASSERT_EQ(n, TestLockStage(ls).get_holders());
  }

  void expect_requesting_actions_to_be(LockStage& ls, const LockStage::RequestingActions& exp) {
    ASSERT_EQ(exp, TestLockStage(ls).get_requesters());
  }

  void expect_next_stage_to_be(LockStage& ls, LockStage* ns) {
    ASSERT_EQ(ns, TestLockStage(ls).get_next_stage());
  }

  void expect_lock_type_to_be(LockStage& ls, LockType lt) {
    ASSERT_EQ(lt, TestLockStage(ls).get_lock_type());
  }

  void expect_lock_stages_to_equal(LockStage& ls1, LockStage& ls2) {
    ASSERT_EQ(TestLockStage(ls1), TestLockStage(ls2));
  }
};

TEST_F(LockStageTest, constructorTests) {
  LockStage ls1;
  expect_holders_to_be(ls1, 0);
  expect_requesting_actions_to_be(ls1, LockStage::RequestingActions({}));
  expect_next_stage_to_be(ls1, nullptr);
  expect_lock_type_to_be(ls1, LockType::shared);

  std::shared_ptr<TestAction> requester = TestAction::make_test_action_with_test_txn({}, {});
  LockStage ls2(
      {requester},
      LockType::exclusive,
      &ls1);
  expect_holders_to_be(ls2, 1);
  expect_requesting_actions_to_be(ls2, {requester});
  expect_next_stage_to_be(ls2, &ls1);
  expect_lock_type_to_be(ls2, LockType::exclusive);
}

// Adding more shared actions to a shared lock stage is OK. An attempt 
// to add an exlusive action to a shared lock stage should fail.
TEST_F(LockStageTest, addStageToSharedTest) {
  LockStage ls1;
  // insertion of a single shared stage to an empty lockstage
  ASSERT_TRUE(ls1.add_to_stage(
        TestAction::make_test_action_with_test_txn({}, {}),
        LockType::shared));
  expect_lock_type_to_be(ls1, LockType::shared);
  expect_next_stage_to_be(ls1, nullptr);
  ASSERT_EQ(ls1.get_requesters().size(), 1);

  // insertion of another shared stage to the same lockstage
  ASSERT_TRUE(ls1.add_to_stage(
        TestAction::make_test_action_with_test_txn({}, {}),
        LockType::shared));
  expect_lock_type_to_be(ls1, LockType::shared);
  expect_next_stage_to_be(ls1, nullptr);
  ASSERT_EQ(ls1.get_requesters().size(), 2);

  // attempt to insert an exlusive stage to the same lockstage
  LockStage expected(ls1);
  ASSERT_FALSE(ls1.add_to_stage(
        TestAction::make_test_action_with_test_txn({}, {}),
        LockType::exclusive));
  // the stage should not have changed
  expect_lock_stages_to_equal(expected, ls1); 
}

// Adding exclusive action to an empty lock stage is OK. Adding anything
// to an exclusive lock stage should fail.
TEST_F(LockStageTest, addStageToExclusiveTest) {
  LockStage ls1;
  ASSERT_TRUE(ls1.add_to_stage(
        TestAction::make_test_action_with_test_txn({}, {}),
        LockType::exclusive));
  ASSERT_EQ(TestLockStage(ls1).get_lock_type(), LockType::exclusive);
  ASSERT_EQ(ls1.get_requesters().size(), 1);

  // no matter what we try to add, we should fail and the lockstage will
  // not change
  LockStage expected(ls1);
  ASSERT_FALSE(ls1.add_to_stage(
        TestAction::make_test_action_with_test_txn({}, {}),
        LockType::exclusive));
  expect_lock_stages_to_equal(expected, ls1); 

  ASSERT_FALSE(ls1.add_to_stage(
        TestAction::make_test_action_with_test_txn({}, {}),
        LockType::shared));
  expect_lock_stages_to_equal(expected, ls1); 
}

TEST_F(LockStageTest, decrement_holdersTest) {
  // lock stage with 100 actions
  LockStage ls1;
  for (unsigned int i = 0; i < 100; i++) 
    ls1.add_to_stage(TestAction::make_test_action_with_test_txn({}, {}), LockType::shared);

  // single threaded decrement works.
  expect_holders_to_be(ls1, 100);
  ASSERT_EQ(99, ls1.decrement_holders());

  // launch two threads to decrement different number, but adding up to 99
  std::thread threads[2];
  threads[0] = std::thread([&ls1](){for(unsigned int i = 0; i < 50; i++) ls1.decrement_holders();});
  threads[1] = std::thread([&ls1](){for(unsigned int i = 0; i < 49; i++) ls1.decrement_holders();});

  threads[0].join();
  threads[1].join();
  expect_holders_to_be(ls1, 0);
} 

TEST_F(LockStageTest, SetNextStage) {
  LockStage ls1;
  LockStage ls2;

  // single-threaded setting works
  ls1.set_next_stage(&ls2);
  expect_next_stage_to_be(ls1, &ls2);

  LockStage ls3;
  // multiple threads leave the state consistent
  std::thread threads[2];
  threads[0] = std::thread([&ls1, &ls2](){for(unsigned int i = 0; i < 100; i++) ls1.set_next_stage(&ls2);});
  threads[1] = std::thread([&ls1, &ls3](){for(unsigned int i = 0; i < 100; i++) ls1.set_next_stage(&ls3);});

  threads[0].join();
  threads[1].join();
  ASSERT_TRUE(
      TestLockStage(ls1).get_next_stage() == &ls2 || 
      TestLockStage(ls1).get_next_stage() == &ls3);
}
