#include "gtest/gtest.h"
#include "batch/txn_factory.h"
#include "test/test_action.h"

class BatchActionAllocatorTest : public testing::Test {
protected:
  const unsigned int batch_size = 100;
  std::unique_ptr<BatchActionAllocator<TestAction>> baa_test_ptr;

  virtual void SetUp() {
    baa_test_ptr = std::make_unique<BatchActionAllocator<TestAction>>(batch_size);
  };
};

TEST_F(BatchActionAllocatorTest, single_allocation_test) {
  TestAction* ta = baa_test_ptr->get_action();

  // default constructed.
  ASSERT_EQ(0, ta->get_id());
  ASSERT_EQ(0, ta->get_readset_size());
  ASSERT_EQ(0, ta->get_writeset_size());
};

TEST_F(BatchActionAllocatorTest, single_batch_allocation_mem_leak_test) {
  // we do NOT save any reference to the actions and the destructor
  // of the baa should still clear the memory correctly.
  for (unsigned int i = 0; i < batch_size; i++) {
    (void) baa_test_ptr->get_action();
  }
};

TEST_F(BatchActionAllocatorTest, over_single_batch_allocation_test) {
  TestAction* ta;
  std::unordered_set<TestAction*> actions;

  for (unsigned int i = 0; i < 4 * batch_size; i++) {
    ta = baa_test_ptr->get_action();
    // default constructed.
    ASSERT_EQ(0, ta->get_id());
    ASSERT_EQ(0, ta->get_readset_size());
    ASSERT_EQ(0, ta->get_writeset_size());
    actions.insert(ta);
  }

  // all of the actions are unique.
  ASSERT_EQ(actions.size(), 4 * batch_size);
};

TEST_F(BatchActionAllocatorTest, over_single_batch_allocation_mem_test) {
  // we do NOT save any reference to the actions and the destructor
  // of the baa should still clear the memory correctly.
  for (unsigned int i = 0; i < 4*batch_size; i++) {
    (void) baa_test_ptr->get_action();
  }

}
