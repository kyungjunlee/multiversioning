#include "batch/scheduler.h"
#include "batch/arr_container.h"
#include "batch/packing.h"
#include "util.h"

#include <cassert>

Scheduler::Scheduler(
    SchedulerThreadManager* manager,
    int m_cpu_number,
    uint64_t thread_id):
  SchedulerThread(manager, m_cpu_number, thread_id)
{};

void Scheduler::StartWorking() {
  while(!is_stop_requested()) {
    TIME_IF_SCHED_DIAGNOSTICS(
      // get the batch actions
      batch_actions = std::move(this->manager->request_input(this));
      TIME_IF_SCHED_DIAGNOSTICS(
        process_batch();, 
        diag.time_creating_schedule.add_sample, tp_2);
      /*
       * TODO: periodically hand_batch_to_execution or
       * executor manager periodically picks packings from schedulers
       */
      this->manager->hand_batch_to_execution(
          this, 
          batch_actions.batch_id, 
          std::move(workloads), 
          std::move(lt));,
      diag.time_per_iteration.add_sample,
      tp_1
     );
  }
};

void Scheduler::Init() {
};

void Scheduler::process_batch() {
  workloads = SchedulerThreadManager::OrderedWorkload(batch_actions.batch.size());
  lt = BatchLockTable();
  ArrayContainer ac(std::move(batch_actions.batch));

  // populate the batch lock table and workloads
  unsigned int curr_workload_item = 0;
  /*
   * TODO: do one-shot-packing; i.e., create as many packings as possible in one iteration
   */
  std::vector<std::unique_ptr<IBatchAction>> packing;
  // std::vector<std::vector<std::unique_ptr<IBatchAction>>> packings;
  // packings = std::move(Packer::get_packings(&ac));

  while (ac.get_remaining_count() != 0) {
    packing = std::move(Packer::get_packing(&ac));
    ac.sort_remaining();

    for (std::unique_ptr<IBatchAction>& act : packing) {
      /*
       * TODO: isn't it creating so many IBatchAction instances here?
       */
      auto act_sptr = std::shared_ptr<IBatchAction>(std::move(act));
      workloads[curr_workload_item++] = act_sptr;
      lt.insert_lock_request(act_sptr);
    }
  }

  assert(curr_workload_item == workloads.size());
};

Scheduler::~Scheduler() {
};

void Scheduler::signal_stop_working() {
  xchgq(&stop_signal, 1);
}

bool Scheduler::is_stop_requested() {
  return stop_signal;
}

void Scheduler::reset() {
  // The only reason why this doesn't have to be locked
  // is that reset should only ever be called when the system
  // has no input. That means that the scheduler thread is awaiting 
  // input or helping to merge and not using the diagnostics. If 
  // that weren't the case, we'd have to lock!
  IF_SCHED_DIAG(
    diag.reset();
  );
} 
