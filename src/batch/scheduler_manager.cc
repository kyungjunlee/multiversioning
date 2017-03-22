#include "batch/scheduler_manager.h"

#include <utility>

SchedulerManager::SchedulerManager(
    SchedulingSystemConfig c,
    ExecutorThreadManager* exec):
  SchedulerThreadManager(exec),
  SchedulingSystem(c)
{
  iq = std::make_unique<InputQueue>();
	current_input_scheduler = 0;
	current_signaling_scheduler = 0;
	current_merging_scheduler = 0;
	create_threads();
};

bool SchedulerManager::inputs_are_ok(
    SchedulerThread* s,
    std::vector<SchedulerState> possible_states_of_s) {
  bool everything_ok = false;
  for (auto& state : possible_states_of_s) {
    everything_ok |= (s->get_state() == state);
  }

  return everything_ok && (s != nullptr);
}

bool SchedulerManager::system_is_initialized() {
  return schedulers.size() > 0;
}

void SchedulerManager::add_action(std::unique_ptr<BatchAction>&& act) {
	iq->push_tail(std::move(act));
};

void SchedulerManager::create_threads() {
  for (int i = 0; 
      i < this->conf.scheduling_threads_count; 
      i++) {
		// TODO: the cpu num must be done better here. Coordination with 
		// execution threads is necessary.
		schedulers.push_back(
			std::make_shared<Scheduler>(this, i));
  }
};

void SchedulerManager::start_working() {
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->StartWorking();
  }
};

void SchedulerManager::init_threads() {
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->Init();
  }
};

SchedulerThread::BatchActions SchedulerManager::request_input(SchedulerThread* s) {
  assert(
      inputs_are_ok(s, 
        {SchedulerState::waiting_for_input, 
        SchedulerState::input}) &&
      system_is_initialized());
  
  // fast track -- there is currently no thread trying to
  // get actions. Notice that we do this only not to bounce cache too much. 
	// If we continued getting current_input_scheduler with a barrier
	// we would likely be doing too much memory operations! The state is thread
	// local.
  uint64_t h = current_input_scheduler;
  barrier();
  if (s != schedulers[h].get()) {
    // there is some other thread getting the actions.
    while (s->get_state() != SchedulerState::input);
  } 

  std::unique_ptr<BatchAction>* act;
	SchedulerThread::BatchActions batch(this->conf.batch_size_act);
  for (unsigned int actionsTaken = 0; 
      actionsTaken < this->conf.batch_size_act; 
      actionsTaken ++) {
    while ((act = this->iq->try_pop_head()) == nullptr);
    batch[actionsTaken] = std::move(*act);
  }

  // move the holder to the next scheduler in turn.
  uint64_t next = (h + 1) % schedulers.size();

  // change the state of the next thread if there is one waiting
	bool cas_success = false;
  if (schedulers[next]->get_state() == SchedulerState::waiting_for_input) {
    cas_success = schedulers[next]->signal_input();
    assert(cas_success);
  }

	// formally increment the current_input_scheduler
  cas_success = cmp_and_swap(
      &current_input_scheduler,
      h,
      next);
  assert(cas_success);

	return batch;
};

void SchedulerManager::signal_exec_threads(
    SchedulerThread* s,
    ExecutorThreadManager::SignalWorkload&& workload) {
  assert(
      inputs_are_ok(
        s,
        {SchedulerState::waiting_to_signal_execution, 
        SchedulerState::signaling_execution}));

  // fast track -- as in request_input!
  uint64_t h = current_signaling_scheduler;
  barrier();
  if (s != schedulers[h].get()) {
    // there is some other thread getting the actions
    while (s->get_state() != SchedulerState::signaling_execution);
  }

  exec_manager->signal_execution_threads(std::move(workload));

  // move the holder to the next scheduler in turn.
  uint64_t next = (h + 1) % schedulers.size();

  bool cas_success = false;
  if (schedulers[next]->get_state() == SchedulerState::waiting_to_signal_execution) {
    cas_success = schedulers[next]->signal_merging();
    assert(cas_success);
  }

  // formally increment the current input scheduler
  cas_success = cmp_and_swap(
      &current_signaling_scheduler,
      h,
      next);
  assert(cas_success);
};

void SchedulerManager::merge_into_global_schedule(
    SchedulerThread* s,
    BatchLockTable&& blt) {
	// TODO
	(void) blt;
  (void) s;
};
