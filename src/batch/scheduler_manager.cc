#include "batch/scheduler_manager.h"
#include "batch/mutex_rw_guard.h"

#include <utility>
#include <algorithm>

ThreadInputQueues::ThreadInputQueues(
    unsigned int thread_number,
    unsigned int batch_size_act):
  input_batch_id(0),
  iq(batch_size_act)
{
  pthread_rwlock_init(&input_lock, NULL);
  queues.resize(thread_number);
};

void ThreadInputQueues::add_action(std::unique_ptr<IBatchAction>&& act) {
  iq.add_action(std::move(act));
};

void ThreadInputQueues::flush_actions() {
  iq.flush();
};

bool ThreadInputQueues::unassigned_input_exists() {
  return iq.is_empty() == false;
};

ThreadInputQueue& ThreadInputQueues::get_my_queue(SchedulerThread* s) {
  return queues[s->get_thread_id()];
};

bool ThreadInputQueues::input_awaits(SchedulerThread* s) {
  return get_my_queue(s).is_empty() == false;
};

SchedulerThreadBatch ThreadInputQueues::get_batch_from_my_queue(
    SchedulerThread* s) {
  auto& my_queue = queues[s->get_thread_id()];
  // busy wait until input from my own queue available.
  while (my_queue.is_empty()) {
    if (s->is_stop_requested()) {
      return SchedulerThreadBatch {
        .batch = std::vector<std::unique_ptr<IBatchAction>>(),
        .batch_id = 0
      };
    }
  }

  SchedulerThreadBatch batch = std::move(my_queue.peek_head());
  my_queue.pop_head();
  return batch;
};

void ThreadInputQueues::assign_inputs(SchedulerThread* s) {
  MutexRWGuard g(&input_lock, LockType::exclusive, true);
  
  if (g.is_locked() == false) {
    // The lock is not granted. There must be a different thread assigning
    // inputs to all of the threads.
    return; 
  }

  // Lock is granted. Assign inputs.
  InputQueue::BatchActions actions;
  unsigned int thread_count = queues.size();
  for (unsigned int i = 0; i < thread_count; i++) {
    auto& cur_queue = queues[i]; 
    if (cur_queue.is_empty() == false) continue;

    while((actions = iq.try_get_action_batch()).size() == 0) {
      // No input to the system available.
      if (input_awaits(s)) {
        // if the distributing thread has unfinished work, stop waiting
        // and continue execution.
        return;
      }

     // Otherwise busy wait until input arrives or until 
     // system is shut down.
     if (s->is_stop_requested()) {
        return;
      }
    }

    // Input to the system exists, assign it.
    cur_queue.push_tail({
        .batch = std::move(actions),
        .batch_id = input_batch_id ++ 
    });
  } 
};

SchedulerManager::SchedulerManager(
    SchedulingSystemConfig c,
    ExecutorThreadManager* exec):
  SchedulerThreadManager(exec),
  SchedulingSystem(c),
  thread_input(
      c.scheduling_threads_count,
      c.batch_size_act),
  handed_batch_id(0),
  pending_batches(c.scheduling_threads_count)
{
  pthread_rwlock_init(&handing_lock, NULL);
	create_threads();
};

bool SchedulerManager::system_is_initialized() {
  return schedulers.size() > 0;
}

void SchedulerManager::add_action(std::unique_ptr<IBatchAction>&& act) {
	thread_input.add_action(std::move(act));
};

void SchedulerManager::flush_actions() {
  thread_input.flush_actions();
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

  if (thread_input.input_awaits(s)) {
    return thread_input.get_batch_from_my_queue(s);
  }
 
  thread_input.assign_inputs(s);
  return thread_input.get_batch_from_my_queue(s);
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
  unsigned exec_number = exec_manager->get_executor_num();
  assert(exec_number > 0);
  ExecutorThreadManager::ThreadWorkloads tw(exec_number);
  
  // initialize the memory all at once.
  unsigned int mod = workload.size() % exec_number;
  unsigned int per_exec_thr_size = workload.size() / exec_number;
  for (unsigned int i = 0; i < tw.size(); i++) {
    // notice that the "+ (mod < i)" takes care of unequal division among thread.
    tw[i].resize(per_exec_thr_size + (i < mod));
  }

  for (unsigned int i = 0; i < workload.size(); i++) {
    tw[i % exec_number][i / exec_number] = workload[i];
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

  auto pass_workloads_on = [this]() {
    // Lock has been granted.
    unsigned int queues_num = pending_batches.pending_queues.size();

    // pass all of the queues and insert into the vector
    for (unsigned int i = 0; i < queues_num; i++) {
      auto& current_queue = pending_batches.pending_queues[i]; 
      while (current_queue.is_empty() == false) {
        sorted_pending_batches.push_back(std::move(current_queue.peek_head()));
        current_queue.pop_head();
      }
    }

    // sort the vector
    std::sort(sorted_pending_batches.begin(), sorted_pending_batches.end(), 
        [](AwaitingBatch& aw1, AwaitingBatch& aw2) {
          return aw1.id < aw2.id;    
      });
    
    // attempt to finalize as many as we can.
    unsigned processed_batches = 0;
    for (; processed_batches < sorted_pending_batches.size(); processed_batches++) {
      auto& curr_awaiting_batch = sorted_pending_batches[processed_batches];
      if (curr_awaiting_batch.id != handed_batch_id) {
        break;
      }
      
      gs->merge_into_global_schedule(std::move(curr_awaiting_batch.blt));
      exec_manager->signal_execution_threads(std::move(curr_awaiting_batch.tw));
      handed_batch_id ++;
    } 

    // erase processed elements.
    if (processed_batches > 0) {
      sorted_pending_batches.erase(
          sorted_pending_batches.begin(), 
          sorted_pending_batches.begin() + processed_batches);
    }
  };

  pass_workloads_on();
  // make sure we don't block.
  if (thread_input.unassigned_input_exists() && 
      thread_input.input_awaits(s) == false &&
      s->is_stop_requested() == false) {
    pass_workloads_on();
  }
};

SchedulerManager::~SchedulerManager() {
  // just to make sure that we are safe on this front.
  stop_working();
};
