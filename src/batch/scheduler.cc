#include "batch/scheduler.h"
#include "batch/arr_container.h"
#include "batch/packing.h"
#include "util.h"

#include <cassert>

Scheduler::Scheduler(SchedulerConfig sc):
    Runnable(sc.m_cpu_number),
    conf(sc),
    state(SchedulerState::waiting_for_input)  
{
  // allocate memory up front.
  batch_actions = 
    std::make_unique<std::vector<std::unique_ptr<BatchAction>>>(
        sc.batch_size_act);
}

void Scheduler::StartWorking() {
  // TODO: implement a flag for killing the thread.
  while(true) {
    // get the batch actions
    signalWaitingForInput();
    conf.input->obtain_batch(this);

    // make a batch schedule
    signalBatchCreation();
    makeBatchSchedule();
    signalWaitingForMerge();
    // TODO: Merge into the global schedule.
    signalMerging(); // This goes away with the above TODO.
    signalWaitingForExecSignal();
    // TODO: Signal the execution threads
    signalExecSignal(); // This goes away with the above TODO
  }
};

void Scheduler::Init() {
};

SchedulerState Scheduler::getState() {
  return state;
};

void Scheduler::makeBatchSchedule() {
  // construct array container from the batch
  ArrayContainer ac(std::move(batch_actions));
  batch_actions = 
    std::make_unique<std::vector<std::unique_ptr<BatchAction>>>(
        conf.batch_size_act);

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

unsigned int Scheduler::getMaxActions() {
  return conf.batch_size_act;
};

void Scheduler::putAction(std::unique_ptr<BatchAction> act) {
  batch_actions->push_back(std::move(act)); 
};

bool Scheduler::changeState(
    SchedulerState nextState, 
    SchedulerState expectedCurrState) {
  // assertion to make sure that we never skip states.
  return cmp_and_swap(
      (uint64_t*) &state,
      (uint64_t) expectedCurrState,
      (uint64_t) nextState);
};

bool Scheduler::signalWaitingForInput() {
  return changeState(
      SchedulerState::waiting_for_input,
      SchedulerState::signaling_execution); 
};

bool Scheduler::signalInput() {
  return changeState(
      SchedulerState::input,
      SchedulerState::waiting_for_input);
};

bool Scheduler::signalBatchCreation() {
  return changeState(
      SchedulerState::batch_creation,
      SchedulerState::input);
}

bool Scheduler::signalWaitingForMerge() {
  return changeState(
      SchedulerState::waiting_to_merge,
      SchedulerState::batch_creation);
};

bool Scheduler::signalMerging() { 
  return changeState(
      SchedulerState::batch_merging,
      SchedulerState::waiting_to_merge);
};

bool Scheduler::signalWaitingForExecSignal() {
  return changeState(
      SchedulerState::waiting_to_signal_execution,
      SchedulerState::batch_merging);
};

bool Scheduler::signalExecSignal() {
  return changeState(
      SchedulerState::signaling_execution,
      SchedulerState::waiting_to_signal_execution);
};
