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

void SchedulerManager::add_action(std::unique_ptr<IBatchAction>&& act) {
	iq->push_tail(std::move(act));
};

void SchedulerManager::set_global_schedule_ptr(IGlobalSchedule* gs) {
  this->gs = gs;
}

void SchedulerManager::create_threads() {
  for (int i = 0; 
      i < this->conf.scheduling_threads_count; 
      i++) {
		schedulers.push_back(
			std::make_shared<Scheduler>(this, conf.first_pin_cpu_id + i));
  }
};

void SchedulerManager::start_working() {
  assert(gs != nullptr);
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->Run();
    scheduler_thread_ptr->WaitInit();
  }
};

void SchedulerManager::init() {
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->Init();
  }
};

void SchedulerManager::stop_working() {
  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->signal_stop_working();
    scheduler_thread_ptr->Join();
  }
}

SchedulerThread::BatchActions SchedulerManager::request_input(SchedulerThread* s) {
  assert(
      s != nullptr &&
      system_is_initialized());
  
  // TODO:
  //    Consider conditional variables here if scheduling is slow.
  uint64_t h;
  do {
    if (s->is_stop_requested()) return SchedulerThread::BatchActions();
    h = current_input_scheduler;
    barrier();
  } while (s != schedulers[h].get());

	SchedulerThread::BatchActions batch(this->conf.batch_size_act);
  for (unsigned int actionsTaken = 0; 
      actionsTaken < this->conf.batch_size_act; 
      actionsTaken ++) {
    while (this->iq->is_empty()) {
      if (s->is_stop_requested())  return SchedulerThread::BatchActions();
    }
    batch[actionsTaken] = std::move(this->iq->peek_head());
    this->iq->pop_head();
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
    OrderedWorkload&& workload) {
  assert(
      s != nullptr &&
      system_is_initialized());

  // convert OrderedWorkloads to ThreadWorkloads.
  std::vector<OrderedWorkload> tw(exec_manager->get_executor_num());
  for (unsigned int i = 0; i < workload.size(); i++) {
    tw[i % tw.size()].push_back(workload[i]);
  }

  // TODO:
  //    Consider conditional variables here if scheduling is slow.
  uint64_t h;
  do {
    if (s->is_stop_requested()) return;
    h = current_signaling_scheduler;
    barrier();
  } while (s != schedulers[h].get());

  exec_manager->signal_execution_threads(std::move(tw));

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
    if (s->is_stop_requested()) return;
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

SchedulerManager::~SchedulerManager() {
  // just to make sure that we are safe on this front.
  stop_working();
};
