#include "batch/scheduler.h"
#include "batch/arr_container.h"
#include "batch/packing.h"
#include "util.h"

#include <cassert>

Scheduler::Scheduler(
    SchedulerThreadManager* manager,
    int m_cpu_number):
  SchedulerThread(manager, m_cpu_number)
{};

void Scheduler::StartWorking() {
  while(!is_stop_requested()) {
    // get the batch actions
    batch_actions = std::make_unique<BatchActions>(
        std::move(this->manager->request_input(this)));
    process_batch();
    this->manager->merge_into_global_schedule(this, std::move(lt));
    this->manager->signal_exec_threads(this, std::move(workloads));
  }
};

void Scheduler::Init() {
};

void Scheduler::process_batch() {
  workloads = std::vector<std::shared_ptr<IBatchAction>>(batch_actions->size());
  lt = BatchLockTable();
  ArrayContainer ac(std::move(batch_actions));

  // populate the batch lock table and workloads
  unsigned int curr_workload_item = 0;
  std::vector<std::unique_ptr<IBatchAction>> packing;
  while (ac.get_remaining_count() != 0) {
    // get packing
    packing = Packer::get_packing(&ac);
    ac.sort_remaining();
    // translate a packing into lock request
    for (std::unique_ptr<IBatchAction>& act : packing) {
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
