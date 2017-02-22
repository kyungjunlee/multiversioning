#include "batch/input_queue.h"
#include "batch/scheduler.h"

#include <cassert>

InputQueue::InputQueue() {};

void InputQueue::initialize(std::vector<Scheduler*> schedulers) {
  assert(schedulers.size() > 0);
  this->schedulers = schedulers;
  holder = 0;
}

// TODO:
//    - Make sure that one doesn't wait for the batch too long 
//    by using a timer. For now we assume high-load within the 
//    simulations so that timeout never happens.
//    - Profile this to see whether we need "batch dequeues" to
//    expedite the process of obtaining a batch.
//    - Implement a waiting queue for scheduling threads.
void InputQueue::obtain_batch(Scheduler* s) {
  // the input queue has been initialized
  assert(schedulers.size() > 0);
  assert(
      s->getState() == SchedulerState::waiting_for_input ||
      s->getState() == SchedulerState::input);
  
  // fast track -- there is currently no thread trying to
  // get actions
  uint64_t h = holder;
  barrier();
  if (s != schedulers[h]) {
    // there is some other thread getting the actions.
    while (s->getState() != SchedulerState::input);
  } 

  std::unique_ptr<BatchAction>* act;
  for (unsigned int actionsTaken = 0; 
      actionsTaken < s->getMaxActions(); 
      actionsTaken ++) {
    while ((act = try_pop_head()) == nullptr);
    s->putAction(std::move(*act));
  }

  // move the holder to the next scheduler in turn.
  h = holder;
  barrier();
  uint64_t next = (h + 1) % schedulers.size();

  // signal only if the next thread is already waiting.
  bool cas_success = false;
  if (schedulers[next]->getState() == SchedulerState::waiting_for_input) {
    cas_success = schedulers[next]->signalInput();
    assert(cas_success);
  }

  cas_success = cmp_and_swap(
      &holder,
      h,
      next);
  assert(cas_success);
};
