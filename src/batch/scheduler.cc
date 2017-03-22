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
  // TODO: implement a flag for killing the thread.
  while(true) {
    // get the batch actions
    batch_actions = std::make_unique<BatchActions>(
        std::move(this->manager->request_input(this)));
    make_batch_schedule();
    // TODO: Merge into the global schedule.
    // TODO: Signal the execution threads
  }
};

void Scheduler::Init() {
};

void Scheduler::make_batch_schedule() {
  // construct array container from the batch
  ArrayContainer ac(std::move(batch_actions));
  std::vector<std::unique_ptr<BatchActionInterface>> packing;
  while (ac.get_remaining_count() != 0) {
    // get packing
    packing = Packer::get_packing(&ac);
    ac.sort_remaining();
    // translate a packing into lock request
    for (std::unique_ptr<BatchActionInterface>& act : packing) {
      lt.insert_lock_request(std::shared_ptr<BatchActionInterface>(std::move(act)));
    }
  }
};
