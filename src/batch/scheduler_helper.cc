#include "batch/scheduler.h"
#include "batch/arr_container.h"
#include "batch/packing.h"
#include "util.h"

#include <cassert>

SchedulerHelper::SchedulerHelper(
    SchedulerThreadManager* manager,
    int m_cpu_number,
    uint64_t thread_id):
  SchedulerHelperThread(manager, m_cpu_number, thread_id)
{};

void SchedulerHelper::StartWorking() {
  while(!is_stop_requested()) {
    /*
     * process_created_batches will perform following:
     *  1. collect awaiting batches
     *  2. merge them into global schedulers
     *  3. send ready packings to executing threads
     */
    this->manager->process_created_batches();
  }
};

void SchedulerHelper::Init() {
};

SchedulerHelper::~SchedulerHelper() {
};

void SchedulerHelper::signal_stop_working() {
  xchgq(&stop_signal, 1);
}

bool SchedulerHelper::is_stop_requested() {
  return stop_signal;
}

void SchedulerHelper::reset() {
  // The only reason why this doesn't have to be locked
  // is that reset should only ever be called when the system
  // has no input. That means that the scheduler thread is awaiting 
  // input or helping to merge and not using the diagnostics. If 
  // that weren't the case, we'd have to lock!
  IF_SCHED_DIAG(
    diag.reset();
  );
} 
