#include "batch/scheduler_manager.h"
#include "batch/mutex_rw_guard.h"

#include <utility>

SchedulerManager::SchedulerManager(
    SchedulingSystemConfig c,
    ExecutorThreadManager* exec):
  SchedulerThreadManager(exec),
  SchedulingSystem(c),
  input_batch_id(0),
  handed_batch_id(0),
  pending_batches(c.scheduling_threads_count)
{
  iq = std::make_unique<BatchedInputQueue>(c.batch_size_act);
  pthread_rwlock_init(&input_lock, NULL);
  pthread_rwlock_init(&handing_lock, NULL);
	create_threads();
};

bool SchedulerManager::system_is_initialized() {
  return schedulers.size() > 0;
}

void SchedulerManager::add_action(std::unique_ptr<IBatchAction>&& act) {
	iq->add_action(std::move(act));
};

void SchedulerManager::flush_actions() {
  iq->flush();
};

void SchedulerManager::set_global_schedule_ptr(IGlobalSchedule* gs) {
  this->gs = gs;
}

void SchedulerManager::create_threads() {
  for (int i = 0; 
      i < this->conf.scheduling_threads_count; 
      i++) {
		schedulers.push_back(
			std::make_shared<Scheduler>(this, conf.first_pin_cpu_id + i, i));
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

SchedulerThreadBatch SchedulerManager::request_input(SchedulerThread* s) {
  assert(
    s != nullptr &&
    system_is_initialized());
  
  // blocks until lock is granted.
  MutexRWGuard g(&input_lock, LockType::exclusive, true);
  while(g.is_locked() == false && !s->is_stop_requested()) {
    g.write_trylock();
  } 

  // avoid returning inconsistent data
  if (g.is_locked() == false) {
    return SchedulerThreadBatch {
      .batch = std::vector<std::unique_ptr<IBatchAction>>(),
      .batch_id = 0
    };
  }

  assert(g.is_locked() == true);
  InputQueue::BatchActions batch;
  while ((batch = this->iq->try_get_action_batch()).size() == 0) {
    if (s->is_stop_requested()) {
      return SchedulerThreadBatch {
        .batch = std::vector<std::unique_ptr<IBatchAction>>(),
        .batch_id = 0
      };
    }
  }

  return SchedulerThreadBatch ({
    .batch = std::move(batch),
    .batch_id = input_batch_id ++ 
  });
};

void SchedulerManager::hand_batch_to_execution(
    SchedulerThread* s,
    uint64_t batch_id,
    OrderedWorkload&& workload,
    BatchLockTable&& blt) {
  assert(
      s != nullptr &&
      system_is_initialized());

  // convert the given workload.batch to format accepted by the execution system.
  ExecutorThreadManager::ThreadWorkloads tw(exec_manager->get_executor_num());
  for (unsigned int i = 0; i < workload.size(); i++) {
    tw[i % tw.size()].push_back(workload[i]);
  }

  // we don't need a lock to access s's pending batches queue. That is because
  // we are the only producer and consumer must hold the handing_lock. 
  pending_batches.pending_queues[s->get_thread_id()].push_tail({
    .id = batch_id,
    .blt = std::move(blt),
    .tw = std::move(tw)
  });

  // Attempt to begin handing batches if one may get the lock.
  MutexRWGuard g(&handing_lock, LockType::exclusive, true);
  if (g.is_locked() == false) return;

  assert(g.is_locked() == true);
  assert(pending_batches.pending_queues.size() == schedulers.size());
  // Lock has been granted.
  unsigned int failed_attempts = 0;
  unsigned int queues_num = pending_batches.pending_queues.size();
  unsigned int round_robin_counter = s->get_thread_id();
  // continue until we have attempted everything round-robin and failed on each.
  while (failed_attempts <= queues_num) {
    // move the counters ahead assuming failure.
    auto& current_queue = pending_batches.pending_queues[round_robin_counter];
    round_robin_counter ++;
    round_robin_counter %= queues_num;
    failed_attempts ++;

    if (current_queue.is_empty()) continue;
    if (current_queue.peek_head().id != handed_batch_id) continue;

    auto current_head = current_queue.peek_head();
    // the current batch is the one that should be handed over!
    gs->merge_into_global_schedule(std::move(current_head.blt));
    exec_manager->signal_execution_threads(std::move(current_head.tw));

    // advance the current queue
    current_queue.pop_head();
    // advance the handed counter
    handed_batch_id ++;
    // reset failures
    failed_attempts = 0;
  }
};

SchedulerManager::~SchedulerManager() {
  // just to make sure that we are safe on this front.
  stop_working();
};
