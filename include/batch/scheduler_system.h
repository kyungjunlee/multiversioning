#ifndef SCHEDULER_SYSTEM_H_
#define SCHEDULER_SYSTEM_H_

#include "batch/global_schedule.h"
#include "batch/batch_action_interface.h"
#include "batch/db_storage_interface.h"

#include <memory>

// Scheduler System Config 
//
//    Used to configure the scheduling system within the system.
struct SchedulingSystemConfig {
  uint32_t scheduling_threads_count;
  uint32_t batch_size_act;
  uint32_t batch_length_sec;
  // we pin threads within scheduling system sequentially 
  // starting at this cpu.
  uint32_t first_pin_cpu_id;
  uint32_t num_table_merging_shard;
};

// Scheduling System
//  
//    Represents the scheduling system. This is what the supervisor class
//    thinks scheduling system is. The only interactions between the 
//    supervisor and the scheduling system are described by this interface.
class SchedulingSystem {
protected:
  SchedulingSystemConfig conf;
  DBStorageConfig db_conf;
public:
  SchedulingSystem(
      SchedulingSystemConfig c,
      DBStorageConfig db_c): 
    conf(c),
    db_conf(db_c)
  {};

  // Add actions to the system. The effect may be delayed from the perspective
  // of scheduling threads if the implementation of scheduling system provides
  // batching. Use flush_actions() to enforce visibility to the scheduling threads.
  virtual void add_action(IBatchAction* act) = 0;
  // Enforce flushing of transactions to scheduling threads.
  virtual void flush_actions() = 0;
  virtual void set_global_schedule_ptr(IGlobalSchedule* gs) = 0;
  virtual void start_working() = 0;
  virtual void init() = 0;
  virtual void reset() = 0;
  virtual void stop_working() = 0;

  virtual ~SchedulingSystem() {};
};

#endif // SCHEDULER_SYSTEM_H_
