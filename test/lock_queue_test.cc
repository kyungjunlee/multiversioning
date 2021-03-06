#include "gtest/gtest.h"
#include "batch/lock_queue.h"
#include "test/test_lock_stage.h"
#include "util.h"

class LockQueueTest : public testing::Test {
protected:
  std::vector<std::shared_ptr<TestLockStage>> get_test_lock_stages(unsigned int num) {
    std::vector<std::shared_ptr<TestLockStage>> tlss; 
    for (unsigned int i = 0; i < num; i++) {
      tlss.push_back(std::make_shared<TestLockStage>());
    }

    return tlss;
  }
};

TEST_F(LockQueueTest, BatchLockQueue_non_concurrent_push_tailTest) {
  BatchLockQueue blq;
  auto testLockStages = get_test_lock_stages(100);
  for (unsigned int i = 0; i < 100; i ++) {
    blq.non_concurrent_push_tail(testLockStages[i]);

    // the tail moves while the head remains.
    ASSERT_EQ(testLockStages[i], *blq.peek_tail());
    ASSERT_EQ(testLockStages[0], *blq.peek_head());
  }

  // the queue is well formed
  std::shared_ptr<BatchLockQueue::QueueElt> curr = blq.peek_head_elt();
  for (unsigned int i = 0; i < 100; i++) {
    if (i != 99) ASSERT_EQ(testLockStages[i], *curr->get_contents());

    curr = curr->get_next_elt();
  }

  ASSERT_EQ(nullptr, blq.peek_tail_elt()->get_next_elt());
} 
