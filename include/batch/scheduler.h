#ifndef BATCH_SCHEDULER_H_
#define BATCH_SCHEDULER_H_

#include "batch/batch_action_interface.h"
#include "batch/lock_table.h"
#include "batch/container.h"
#include "batch/scheduler_thread_manager.h"
#include "batch/scheduler_thread.h"

//  Scheduler
//
//    The implementation of a scheduler thread. This is the part of the system
//    that does the actual work of creating batch schedules, merging it within 
//    the global schedule etc.
//
//      TODO:
//          Tests for:
//            - StartWorking 
//            - Init
class Scheduler : public SchedulerThread {
private:
  uint64_t thread_id;
public:
  Scheduler(
      SchedulerThreadManager* manager,
      int m_cpu_number,
      uint64_t thread_id);

  std::unique_ptr<SchedulerThreadBatch> batch_actions;
  BatchLockTable lt;
  SchedulerThreadManager::OrderedWorkload workloads;

  /*
   *  Has two basic effects:
   *    Populates the batch lock table and workloads variables.
   */
  void process_batch();
  
  // override SchedulerThread interface
  void signal_stop_working() override;
  bool is_stop_requested() override;

  // override Runnable interface
  void StartWorking() override;
  void Init() override;

  virtual ~Scheduler();
};

#endif // BATCH_SCHEDULER_H_
