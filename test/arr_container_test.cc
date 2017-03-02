#include <gtest/gtest.h>
#include <batch/arr_container.h>
#include <test/test_txn.h>
#include <test/test_action.h>

#include <iostream>
#include <vector>
#include <memory>

// TODO: 
//    Extract the action prep logic to a utility class / file.
class ArrayContainerTest : public testing::Test {
protected:
  unsigned int ACTIONS_NUM = 100;
  float WRITE_PERC_IN_ACTION = 0.5;

  std::unique_ptr<ArrayContainer> testContainer;

  virtual void SetUp() {
    // prep the actions
    std::unique_ptr<ArrayContainer::actions_vec> actions 
      = std::make_unique<ArrayContainer::actions_vec>();
    
    for (unsigned int i = 0; i < ACTIONS_NUM; i++) {
      std::unique_ptr<TestAction> ta = std::make_unique<TestAction>(new TestTxn());
      
      // add i*WRITE_PERC_IN_ACTION keys to write and the rest to read sets.
      for (unsigned int j = 0; j < (unsigned int) (i * WRITE_PERC_IN_ACTION); j++) {
        ta->add_write_key(j);
      }

      for (unsigned int j = i * WRITE_PERC_IN_ACTION; j < i; j++) {
        ta->add_read_key(j);
      }

      actions->push_back(std::move(ta));
    }
    
    // make the container
    testContainer = std::make_unique<ArrayContainer>(std::move(actions));
  }  
};

// Freshly created container must:
//    Contain correct number of elts
//    Be sorted
TEST_F(ArrayContainerTest, traversalAndPeekAfterCreationTest) {
  for (unsigned int i = 0; i < ACTIONS_NUM; i++) {
    auto act = testContainer->peek_curr_elt();
    ASSERT_EQ(i, act->get_readset_size() + act->get_writeset_size())
       << "Expecting ith action to have i locks.";
    testContainer->advance_to_next_elt();
  }

  EXPECT_TRUE(nullptr == testContainer->peek_curr_elt());
}

TEST_F(ArrayContainerTest, eltTakingAndRemaningCountTest) {
  // remove every other element.
  for (unsigned int i = 0; i < ACTIONS_NUM; i++) {
    if ((i % 2) == 0) {
      EXPECT_EQ(100 - i / 2, testContainer->get_remaining_count());
      auto min = testContainer->take_curr_elt();
      EXPECT_EQ(100 - i/2 - 1, testContainer->get_remaining_count());
    } else {
      testContainer->advance_to_next_elt();
    }      
  }
}

TEST_F(ArrayContainerTest, sortRemainingTest) {
  // remove every other elt.
  for (unsigned int i = 0; i < ACTIONS_NUM; i++) {
    if ((i%2) == 0) testContainer->take_curr_elt();
    else testContainer->advance_to_next_elt();
  }

  // we should have reached the end of the array.
  EXPECT_TRUE(nullptr == testContainer->peek_curr_elt());
  EXPECT_EQ(ACTIONS_NUM / 2, testContainer->get_remaining_count());

  // sort and see that the result is correct 
  testContainer->sort_remaining();
  for (unsigned int i = 1; i < ACTIONS_NUM; i += 2) {
    auto act = testContainer->peek_curr_elt();
    EXPECT_EQ(i, act->get_readset_size() + act->get_writeset_size());
    testContainer->advance_to_next_elt();
  }

  // we should have traversed the whole container.
  EXPECT_TRUE(nullptr == testContainer->peek_curr_elt());
}

TEST_F(ArrayContainerTest, emptyListOps) {
  auto assertContainerEmpty = [this](){
    EXPECT_EQ(0, testContainer->get_remaining_count());
    EXPECT_TRUE(nullptr == testContainer->peek_curr_elt());
  };

  // remove all elts
  for (unsigned int i = 0; i < ACTIONS_NUM; i++)
    testContainer->take_curr_elt();

  // should be empty
  assertContainerEmpty();

  testContainer->sort_remaining();

  // should still be empty
  assertContainerEmpty();

  // "taking" a new elt should just give us nullptr. Should not 
  // change the fact that its empty!
  ASSERT_TRUE(nullptr == testContainer->take_curr_elt());
  assertContainerEmpty();
}
