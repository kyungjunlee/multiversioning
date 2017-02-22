#include "gtest/gtest.h"
#include "batch/input_queue.h"
#include "test/test_scheduler.h"
#include "test/test_action.h"

#include <vector>
#include <cstdint>
#include <utility>
#include <thread>

class InputQueueTest : 
  public testing::Test,
  public testing::WithParamInterface<int> {
protected:
  std::vector<Scheduler*> schedulers;
  std::shared_ptr<InputQueue> iq;
  const uint32_t batch_size = 100;
  const uint32_t schedulers_set_up = 3;
  const uint32_t actions_prepped = batch_size * schedulers_set_up;

  virtual void SetUp() {
    iq = std::make_shared<InputQueue>();

    for (uint32_t i = 0; i < schedulers_set_up; i++) {
      schedulers.push_back(
        new TestScheduler({
          batch_size,
          0, // we do not use timeout for now
          0, // we ignore this in tests. Does not matter.
          iq.get()})); 
    }

    for (unsigned int i = 0; i < actions_prepped; i++) {
      iq->push_tail(std::move(std::make_unique<TestAction>(
          *TestAction::make_test_action_with_test_txn({}, {}, i))));
    }
    iq->initialize(schedulers);
  };
};

void assertActionsAdded(
    TestScheduler* s, 
    unsigned int line, 
    unsigned int from = 0, 
    unsigned int to = 99) {
  TestAction* currAction = nullptr;
  
  for (unsigned int i = from; i <= to; i++) {
    currAction = static_cast<TestAction*>(s->putActions[i - from].get());
    ASSERT_EQ(i, currAction->get_id()) <<
      "Error within test starting at line: " << line;
  }
}

TEST_F(InputQueueTest, obtain_batchNonConcurrentTest) {
  iq->obtain_batch(schedulers[0]);
  TestScheduler* s = static_cast<TestScheduler*>(schedulers[0]);

  ASSERT_EQ(100, s->putActions.size());
  assertActionsAdded(s, __LINE__);
} 

void runConcurrentTest(
    uint32_t number_of_threads, 
    uint32_t batch_size,
    std::vector<Scheduler*>& schedulers,
    std::shared_ptr<InputQueue> iq,
    int line) {
  std::thread threads[number_of_threads];
  for (int i = number_of_threads - 1; i >= 0; i--) {
    threads[i] = std::thread([&schedulers, &iq, i](){
        iq->obtain_batch(schedulers[i]);
    });
  }

  for (unsigned int i = 0; i < number_of_threads; i++) threads[i].join();
  
  TestScheduler* s;
  for (unsigned int i = 0; i < number_of_threads; i++) {
    s = static_cast<TestScheduler*>(schedulers[i]);

    ASSERT_EQ(batch_size, s->putActions.size());
    assertActionsAdded(s, line, batch_size * i, (i+1)*batch_size - 1);
  }
};

// obtain batch Concurrent No State Overflow Test
//    Does not check whether the state of schedulers not yet waiting
//    is changed.
TEST_P(InputQueueTest, obtain_batchConcurrentNSOFTest) {
  runConcurrentTest(
      schedulers_set_up - 1, 
      batch_size,
      schedulers,
      iq,
      __LINE__); 
};

// obtain batch Concurrent State Overflow Test
//    Makes sure that the state of a thead that is not waiting 
//    for input does not get changed preemptively.
TEST_P(InputQueueTest, obtain_batchConcurrentSOFTest) {
  runConcurrentTest(
      schedulers_set_up, 
      batch_size,
      schedulers, 
      iq,
      __LINE__); 
};

INSTANTIATE_TEST_CASE_P(
    RerunsForEdgeCond,
    InputQueueTest,
    testing::Range(1, 50));
