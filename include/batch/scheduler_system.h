#ifndef SCHEDULER_SYSTEM_H_
#define SCHEDULER_SYSTEM_H_

#include "batch/global_schedule.h"
#include "batch/batch_action_interface.h"

#include <memory>

// Scheduler System Config 
//
//    Used to configure the scheduling system within the system.
struct SchedulingSystemConfig {
  uint32_t scheduling_threads_count;
  uint32_t batch_size_act;
  uint32_t batch_length_sec;
};

// Scheduling System
//  
//    Represents the scheduling system. This is what the supervisor class
//    thinks scheduling system is. The only interactions between the 
//    supervisor and the scheduling system are described by this interface.
class SchedulingSystem {
protected:
  SchedulingSystemConfig conf;
public:
  SchedulingSystem(SchedulingSystemConfig c): conf(c) {};

  virtual void add_action(std::unique_ptr<BatchActionInterface>&& act) = 0;
  virtual void set_global_schedule_ptr(GlobalScheduleInterface* gs) = 0;
  virtual void start_working() = 0;
  virtual void init() = 0;
};

#endif // SCHEDULER_SYSTEM_H_
