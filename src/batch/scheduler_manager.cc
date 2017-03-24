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

bool SchedulerManager::system_is_initialized() {
  return schedulers.size() > 0;
}

void SchedulerManager::add_action(std::unique_ptr<BatchAction>&& act) {
	iq->push_tail(std::move(act));
};

void SchedulerManager::set_global_schedule_ptr(GlobalScheduleInterface* gs) {
  this->gs = gs;
}

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
  assert(gs != nullptr);
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->StartWorking();
  }
};

void SchedulerManager::init() {
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->Init();
  }
};

SchedulerThread::BatchActions SchedulerManager::request_input(SchedulerThread* s) {
  assert(
      s != nullptr &&
      system_is_initialized());
  
  // TODO:
  //    Consider conditional variables here if scheduling is slow.
  uint64_t h;
  do {
    h = current_input_scheduler;
    barrier();
  } while (s != schedulers[h].get());

  std::unique_ptr<BatchAction>* act;
	SchedulerThread::BatchActions batch(this->conf.batch_size_act);
  for (unsigned int actionsTaken = 0; 
      actionsTaken < this->conf.batch_size_act; 
      actionsTaken ++) {
    while ((act = this->iq->try_pop_head()) == nullptr);
    batch[actionsTaken] = std::move(*act);
  }

  // formally increment the current_input_scheduler
	bool cas_success = false;
  cas_success = cmp_and_swap(
      &current_input_scheduler,
      h,
      (h + 1) % schedulers.size());
  assert(cas_success);

	return batch;
};

// TODO:
//    Tests for signal_exec_threads
void SchedulerManager::signal_exec_threads(
    SchedulerThread* s,
    ExecutorThreadManager::SignalWorkload&& workload) {
  assert(
      s != nullptr &&
      system_is_initialized());

  // TODO:
  //    Consider conditional variables here if scheduling is slow.
  uint64_t h;
  do {
    h = current_signaling_scheduler;
    barrier();
  } while (s != schedulers[h].get());

  exec_manager->signal_execution_threads(std::move(workload));

  // formally increment the current signaling scheduler
  bool cas_success = cmp_and_swap(
      &current_signaling_scheduler,
      h,
      (h+1) % schedulers.size());
  assert(cas_success);
};

void SchedulerManager::merge_into_global_schedule(
    SchedulerThread* s,
    BatchLockTable&& blt) {
  assert(s != nullptr);

  uint64_t h;
  do {
    h = current_merging_scheduler;
    barrier();
  } while (s != schedulers[h].get());

  gs->merge_into_global_schedule(std::move(blt));

  // formally increment the current merging scheduler
  bool cas_success = cmp_and_swap(
      &current_merging_scheduler,
      h,
      (h+1) % schedulers.size());
  assert(cas_success);
};
