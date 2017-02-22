#include "gtest/gtest.h"
#include "batch/scheduler.h"
#include "test/test_input_queue.h"

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

  virtual void SetUp() {
    iq = std::make_shared<TestInputQueue>();
    SchedulerConfig config = {
      batch_size,
      batch_length,
      m_cpu_num,
      iq.get()
    };
    s = std::make_shared<Scheduler>(config);
  };
};

TEST_F(SchedulerTest, LegalStateTransitionsTest) {
  auto checkState = [this](SchedulerState expected, int line) {
    ASSERT_EQ(this->s->getState(), expected) << "from line " << line;
  };
  checkState(SchedulerState::waiting_for_input, __LINE__);
  s->signalInput();
  checkState(SchedulerState::input, __LINE__);
  s->signalBatchCreation();
  checkState(SchedulerState::batch_creation, __LINE__);
  s->signalWaitingForMerge();
  checkState(SchedulerState::waiting_to_merge, __LINE__);
  s->signalMerging();
  checkState(SchedulerState::batch_merging, __LINE__);
  s->signalWaitingForExecSignal();
  checkState(SchedulerState::waiting_to_signal_execution, __LINE__);
  s->signalExecSignal();
  checkState(SchedulerState::signaling_execution, __LINE__);
  s->signalWaitingForInput();
  checkState(SchedulerState::waiting_for_input, __LINE__);
};

TEST_F(SchedulerTest, IllegalStateTransitionsTest) {
  typedef bool (Scheduler::*signalFunction)();
  std::vector<std::pair<signalFunction, SchedulerState>> legal_pairs = {
    {&Scheduler::signalInput, SchedulerState::waiting_for_input},
    {&Scheduler::signalBatchCreation, SchedulerState::input},
    {&Scheduler::signalWaitingForMerge, SchedulerState::batch_creation},
    {&Scheduler::signalMerging, SchedulerState::waiting_to_merge},
    {&Scheduler::signalWaitingForExecSignal, SchedulerState::batch_merging},
    {&Scheduler::signalExecSignal, SchedulerState::waiting_to_signal_execution},
    {&Scheduler::signalWaitingForInput, SchedulerState::signaling_execution}
  };

  // check that all transitions that do not expect the scheduler to be in 
  // "state" cause false return value
  auto checkAllIllegal = [this, &legal_pairs] (SchedulerState state){
    for (auto& elt : legal_pairs) {
      if (elt.second == state) continue;
      
     auto ptr = elt.first;
     EXPECT_FALSE(((*this->s).*ptr)());
    }
  };

  SchedulerState currState;
  for (unsigned int i = static_cast<unsigned int>(SchedulerState::waiting_for_input);
      i < static_cast<unsigned int>(SchedulerState::state_count);
      i ++) {
    currState = static_cast<SchedulerState>(i);
    checkAllIllegal(currState);

    // change the state of the scheduler.
    auto ptr = legal_pairs[i].first;
    ((*this->s).*ptr)();
  }
}
