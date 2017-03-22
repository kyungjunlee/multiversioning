#include "batch/scheduler.h"
#include "batch/arr_container.h"
#include "batch/packing.h"
#include "util.h"

#include <cassert>

Scheduler::Scheduler(
    SchedulerThreadManager* manager,
    int m_cpu_number):
  SchedulerThread(manager, m_cpu_number)
{};

void Scheduler::StartWorking() {
  // TODO: implement a flag for killing the thread.
  while(true) {
    // get the batch actions
    signal_waiting_for_input();
    batch_actions = std::make_unique<BatchActions>(
        std::move(this->manager->request_input(this)));
    // make a batch schedule
    signal_batch_creation();
    make_batch_schedule();
    signal_waiting_for_merge();
    // TODO: Merge into the global schedule.
    signal_merging(); // This goes away with the above TODO.
    signal_waiting_for_exec_signal();
    // TODO: Signal the execution threads
    signal_exec_signal(); // This goes away with the above TODO
  }
};

void Scheduler::Init() {
};

SchedulerState Scheduler::get_state() {
  return this->state;
};

void Scheduler::make_batch_schedule() {
  // construct array container from the batch
  ArrayContainer ac(std::move(batch_actions));

  std::vector<std::unique_ptr<BatchAction>> packing;
  while (ac.get_remaining_count() != 0) {
    // get packing
    packing = Packer::get_packing(&ac);
    ac.sort_remaining();
    // translate a packing into lock request
    for (std::unique_ptr<BatchAction>& act : packing) {
      lt.insert_lock_request(std::shared_ptr<BatchAction>(std::move(act)));
    }
  }
}

bool Scheduler::change_state(
    SchedulerState nextState, 
    SchedulerState expectedCurrState) {
  // assertion to make sure that we never skip states.
  return cmp_and_swap(
      (uint64_t*) &state,
      (uint64_t) expectedCurrState,
      (uint64_t) nextState);
};

bool Scheduler::signal_waiting_for_input() {
  return change_state(
      SchedulerState::waiting_for_input,
      SchedulerState::signaling_execution); 
};

bool Scheduler::signal_input() {
  return change_state(
      SchedulerState::input,
      SchedulerState::waiting_for_input);
};

bool Scheduler::signal_batch_creation() {
  return change_state(
      SchedulerState::batch_creation,
      SchedulerState::input);
}

bool Scheduler::signal_waiting_for_merge() {
  return change_state(
      SchedulerState::waiting_to_merge,
      SchedulerState::batch_creation);
};

bool Scheduler::signal_merging() { 
  return change_state(
      SchedulerState::batch_merging,
      SchedulerState::waiting_to_merge);
};

bool Scheduler::signal_waiting_for_exec_signal() {
  return change_state(
      SchedulerState::waiting_to_signal_execution,
      SchedulerState::batch_merging);
};

bool Scheduler::signal_exec_signal() {
  return change_state(
      SchedulerState::signaling_execution,
      SchedulerState::waiting_to_signal_execution);
};
