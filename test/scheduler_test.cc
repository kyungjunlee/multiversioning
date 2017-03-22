#include "gtest/gtest.h"
#include "batch/scheduler.h"
#include "test/test_scheduler_thread_manager.h"

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
    s = std::make_shared<Scheduler>(
      new TestSchedulerThreadManager(), 0);    
  };
};

TEST_F(SchedulerTest, LegalStateTransitionsTest) {
  auto checkState = [this](SchedulerState expected, int line) {
    ASSERT_EQ(this->s->get_state(), expected) << "from line " << line;
  };
  checkState(SchedulerState::waiting_for_input, __LINE__);
  EXPECT_TRUE(s->signal_input());
  checkState(SchedulerState::input, __LINE__);
  EXPECT_TRUE(s->signal_batch_creation());
  checkState(SchedulerState::batch_creation, __LINE__);
  EXPECT_TRUE(s->signal_waiting_for_merge());
  checkState(SchedulerState::waiting_to_merge, __LINE__);
  EXPECT_TRUE(s->signal_merging());
  checkState(SchedulerState::batch_merging, __LINE__);
  EXPECT_TRUE(s->signal_waiting_for_exec_signal());
  checkState(SchedulerState::waiting_to_signal_execution, __LINE__);
  EXPECT_TRUE(s->signal_exec_signal());
  checkState(SchedulerState::signaling_execution, __LINE__);
  EXPECT_TRUE(s->signal_waiting_for_input());
  checkState(SchedulerState::waiting_for_input, __LINE__);
};

TEST_F(SchedulerTest, IllegalStateTransitionsTest) {
  typedef bool (Scheduler::*signalFunction)();
  std::vector<std::pair<signalFunction, SchedulerState>> legal_pairs = {
    {&Scheduler::signal_input, SchedulerState::waiting_for_input},
    {&Scheduler::signal_batch_creation, SchedulerState::input},
    {&Scheduler::signal_waiting_for_merge, SchedulerState::batch_creation},
    {&Scheduler::signal_merging, SchedulerState::waiting_to_merge},
    {&Scheduler::signal_waiting_for_exec_signal, SchedulerState::batch_merging},
    {&Scheduler::signal_exec_signal, SchedulerState::waiting_to_signal_execution},
    {&Scheduler::signal_waiting_for_input, SchedulerState::signaling_execution}
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
