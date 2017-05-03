#include "batch/scheduler_manager.h"
#include "batch/mutex_rw_guard.h"

#include <utility>

SchedulerManager::SchedulerManager(
    SchedulingSystemConfig c,
    ExecutorThreadManager* exec):
  SchedulerThreadManager(exec),
  SchedulingSystem(c),
  thread_input(c.scheduling_threads_count),
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
 
  SchedulerThreadBatch batch;
  auto& my_in_queue = thread_input.queues[s->get_thread_id()];
  auto get_batch_from_my_queue = [this, &s, &my_in_queue](){
    SchedulerThreadBatch batch;
    batch = std::move(my_in_queue.peek_head());
    my_in_queue.pop_head();
    return batch;
  };

  // input awaits for us.
  if (my_in_queue.is_empty() == false) {
    return get_batch_from_my_queue();
  }

  MutexRWGuard g(&input_lock, LockType::exclusive, true);
  // lock is not granted. There is a thread that is attempting to give us input.
  if (g.is_locked() == false) {
    // spin until we get a batch
    while(my_in_queue.is_empty()) {
      if (s->is_stop_requested()) {
        return SchedulerThreadBatch {
          .batch = std::vector<std::unique_ptr<IBatchAction>>(),
          .batch_id = 0
        };
      }
    }
    
    return get_batch_from_my_queue();
  }

  // if lock is granted, but current thread got input, it shouldn't 
  // attempt to give input.
  if (my_in_queue.is_empty() == false) {
    g.unlock(); 
    return get_batch_from_my_queue();
  } 

  // lock is granted and our input queue is empty. Give threads input.
  assert(g.is_locked());
  InputQueue::BatchActions actions;
  unsigned int my_id = s->get_thread_id();
  unsigned int thread_count = conf.scheduling_threads_count;
  for (unsigned int i = 0; i < thread_count; i++) {
    // assign input to the current thread last. This avoids the edge condition
    // of blocking the system on the last batch because there is no more input
    // but the current thread tries to give input instead of finishing its own
    // work.
    auto& cur_queue = thread_input.queues[(i + my_id + 1) % thread_count]; 
    if (cur_queue.is_empty() == false) continue;

    // busy wait for input
    while((actions = iq->try_get_action_batch()).size() == 0) {
      if (s->is_stop_requested()) {
        return SchedulerThreadBatch {
          .batch = std::vector<std::unique_ptr<IBatchAction>>(),
          .batch_id = 0
        };
      }
    }

    // assign input.
    cur_queue.push_tail({
        .batch = std::move(actions),
        .batch_id = input_batch_id ++ 
    });
  } 

  return get_batch_from_my_queue();
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
