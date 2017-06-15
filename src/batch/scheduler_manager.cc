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

    // if there is no more input, help handing the load to execution.
    // TODO:
    //  This is a hack... warning with this, might be dangerous.
    SchedulerManager* man = static_cast<SchedulerManager*>(s->manager);
    if (man->thread_input.unassigned_input_exists() == false) {
      man->process_created_batches();
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
    DBStorageConfig db_c,
    ExecutorThreadManager* exec):
  SchedulerThreadManager(exec),
  SchedulingSystem(c, db_c),
  IF_SCHED_MAN_DIAG(diag(c.num_table_merging_shard) COMMA)
  thread_input(
      c.scheduling_threads_count,
      c.batch_size_act),
  pending_batches(c.scheduling_threads_count),
  sorted_pending_batches(),
  records_per_stage(
      db_c.tables_definitions[0].num_records /\
      (c.num_table_merging_shard - 1)),
  merging_queues(c.num_table_merging_shard)
{
  // This system is designed to only work with a single 
  // database storage table. If you want more, please redesign it. 
  // Look at how the db is horizontally sharded for passing to execution.
  assert(db_c.tables_definitions.size() == 1);
  assert(db_c.tables_definitions[0].table_id == 0);

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

ExecutorThreadManager::ThreadWorkloads
SchedulerManager::convert_to_exec_format(OrderedWorkload&& ow) {
  unsigned exec_number = exec_manager->get_executor_num();
  assert(exec_number > 0);
  ExecutorThreadManager::ThreadWorkloads tw(exec_number);
  
  // initialize the memory all at once.
  unsigned int mod = ow.size() % exec_number;
  unsigned int per_exec_thr_size = ow.size() / exec_number;
  for (unsigned int i = 0; i < tw.size(); i++) {
    // notice that the "+ (mod < i)" takes care of unequal division among threads.
    tw[i].resize(per_exec_thr_size + (i < mod));
  }

  for (unsigned int i = 0; i < ow.size(); i++) {
    tw[i % exec_number][i / exec_number] = ow[i];
  }

  return tw;
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

void SchedulerManager::reset() {
  IF_SCHED_MAN_DIAG(
    MutexRWGuard collect_lck(&sorted_pending_batches.lck, LockType::exclusive);
    MutexRWGuard signal_lck(&merging_queues.signaling_lock, LockType::exclusive);
    std::vector<MutexRWGuard> merging_locks;
    for (auto& lck : merging_queues.stage_locks) {
      merging_locks.push_back(MutexRWGuard(&lck, LockType::exclusive));
    }

    // Now we own all of the necessary locks and can reset!
    diag.reset();
  );

  for (auto& scheduler_thread_ptr : schedulers) {
    scheduler_thread_ptr->reset();
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
  register_created_batch(s, batch_id, std::move(workload), std::move(blt));
  process_created_batches();
};

void SchedulerManager::register_created_batch(
    SchedulerThread* s,
    uint64_t batch_id,
    OrderedWorkload&& workload,
    BatchLockTable&& blt) {
  assert(
      s != nullptr &&
      system_is_initialized());

  // we don't need a lock to access s's pending batches queue. That is because
  // we are the only producer and consumer must hold the handing_lock. 
  pending_batches.pending_queues[s->get_thread_id()].push_tail({
    .id = batch_id,
    .blt = std::move(blt),
    .tw = std::move(convert_to_exec_format(std::move(workload)))
  });
};

void SchedulerManager::process_created_batches() {
  collect_awaiting_batches();
  for (unsigned int i = 0; i < conf.num_table_merging_shard; i++) {
    merge_into_global_schedule(i); 
  }
  signal_execution_threads();
};

void SchedulerManager::collect_awaiting_batches() {
  MutexRWGuard g(&sorted_pending_batches.lck, LockType::exclusive, true);
  if (g.is_locked() == false) return;
  assert(g.is_locked());
  
  TIME_IF_SCHED_MAN_DIAGNOSTICS(
    unsigned int queues_num = pending_batches.pending_queues.size(); 
    assert(queues_num == schedulers.size());

    // pass all of the scheduler-thread-local queues and insert
    // batches into the global vector. 
    for (unsigned int i = 0; i < queues_num; i++) {
      auto& current_queue = pending_batches.pending_queues[i];
      while (current_queue.is_empty() == false) {
        sorted_pending_batches
          .batches
          .push_back(std::move(current_queue.peek_head()));
        current_queue.pop_head();
      }
    }

    // sort the vector
    auto& sorted_batches = sorted_pending_batches.batches;
    std::sort(sorted_batches.begin(), sorted_batches.end(), 
        [](AwaitingBatch& aw1, AwaitingBatch& aw2) {
          return aw1.id < aw2.id;    
      });
    
    IF_SCHED_MAN_DIAG(
      // ignore the samples of empty collection queue, since that gives us 
      // no information.
      if (sorted_batches.size() > 0) {
        diag.collection_queue_length.add_sample(sorted_batches.size());
      }
    );
    // attempt to pass as many to merging as we can.
    unsigned processed_batches = 0;
    for (; processed_batches < sorted_batches.size(); processed_batches++) {
      auto& curr_awaiting_batch = sorted_batches[processed_batches];
      if (curr_awaiting_batch.id != sorted_pending_batches.handed_batch_id) {
        break;
      }
      
      merging_queues.merging_stages[0].push_tail(std::move(curr_awaiting_batch)); 
      sorted_pending_batches.handed_batch_id ++;
    } 

    // erase those passed
    if (processed_batches > 0) {
      IF_SCHED_MAN_DIAG(
          diag.collection_queue_processed.add_sample(processed_batches)
      );

      sorted_batches.erase(
        sorted_batches.begin(), 
        sorted_batches.begin() + processed_batches);
    },
    diag.time_collecting_inputs.add_sample,
    tp1);
};

void SchedulerManager::merge_into_global_schedule(unsigned int stage) {
  MutexRWGuard g(
      &merging_queues.stage_locks[stage],
      LockType::exclusive,
      true);
  if (g.is_locked() == false) return;

  TIME_IF_SCHED_MAN_DIAGNOSTICS(
    auto& m_queue = merging_queues.merging_stages[stage];
    RecordKey lo(records_per_stage * stage);
    RecordKey hi(records_per_stage * (stage + 1) - 1);
    AwaitingBatchQueue* next_queue;

    if (stage == merging_queues.merging_stages.size() - 1) {
      // NOTE: 
      //  As mentioned before, we assume only ONE table. In this case we
      //  must have that each table has the same size, which is weaker.
      hi.key = (this->db_conf.tables_definitions[0].num_records  - 1); 
      next_queue = &merging_queues.merged_batches;
    } else {
      next_queue = &merging_queues.merging_stages[stage + 1];
    }

    // we limit the number of batches that a single thread may process
    // on an iteration to make sure that there is no build-up in the 
    // global ordered queue due to lack of continuity in batch numbers.
    unsigned int processed = 0;

    // merge for the current key range for everything that awaits.
    while (m_queue.is_empty() == false) {
      // current awaiting batch
      auto& curr_aw = m_queue.peek_head();
      gs->merge_into_global_schedule_for(curr_aw.blt, lo, hi);
      next_queue->push_tail(std::move(curr_aw));
      m_queue.pop_head();  
      processed ++;

      if (processed > 25) break;
    }
    
    IF_SCHED_MAN_DIAG(
      if (processed > 0) {
        diag.number_merged[stage].add_sample(processed);
      } 
    ),
    diag.time_merging[stage].add_sample,
    t1
  );
};

void SchedulerManager::signal_execution_threads() {
  MutexRWGuard g(
      &merging_queues.signaling_lock,
      LockType::exclusive,
      true);
  if (g.is_locked() == false) return;

  TIME_IF_SCHED_MAN_DIAGNOSTICS(
    auto& m_queue = merging_queues.merged_batches;
    // We must put all awaiting batches that we process into a vector
    // to make sure that their destructors aren't called until the 
    // end of the function when lock is released. Otherwise a huge slowdown
    // ensues!
    std::vector<AwaitingBatch> tmp_aw_batches;
    tmp_aw_batches.resize(300);
   
    while (m_queue.is_empty() == false) {
      auto& curr_aw_batch = m_queue.peek_head();
      exec_manager->signal_execution_threads(std::move(curr_aw_batch.tw));  
      m_queue.pop_head();
      tmp_aw_batches.push_back(std::move(curr_aw_batch));
    }
    
    IF_SCHED_MAN_DIAG(
      if (tmp_aw_batches.size() > 0) {
        diag.number_signaled.add_sample(tmp_aw_batches.size());
      }
    );,
    diag.time_signaling_no_destr.add_sample,
    t1
  );

  g.unlock();
};

SchedulerManager::~SchedulerManager() {
  IF_SCHED_DIAG(
    GlobalSchedulerDiag gsd;
    for (auto& scheduler_thread_ptr : schedulers) {
      // NOTE: This will only work with this particular implementation
      // of scheduler... But I guess that should be alright for now.
      //
      // Otherwise we'd have to define an interface for all of below... FT.
      Scheduler* scheduler_ptr = 
        static_cast<Scheduler*>(scheduler_thread_ptr.get());
      gsd.add_sample(scheduler_ptr->diag);
    }

    gsd.print();
  );
 
  // just to make sure that we are safe on this front.
  stop_working();
  IF_SCHED_MAN_DIAG(diag.print());
};

