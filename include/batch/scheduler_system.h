#ifndef SCHEDULER_SYSTEM_H_
#define SCHEDULER_SYSTEM_H_

#include "batch/input_queue.h"

// Scheduler System Config 
//
//    Used to configure all of the scheduling threads within the system.
//    Since the manager owns, initializes and manages concurrency within
//    the scheduler threads it requires this information.
//
//    This is what the supervisor of the system thinks is the scheduling
//    system. It only cares that it is able to input transactions into
//    the system.
struct SchedulingSystemConfig {
  uint32_t scheduling_threads_count;
  uint32_t batch_size_act;
  uint32_t batch_length_sec;
};

class SchedulingSystem {
protected:
  SchedulingSystemConfig conf;
public:
  SchedulingSystem(SchedulingSystemConfig c): conf(c) {};
  
  std::unique_ptr<InputQueue> iq;

  virtual void add_action(std::unique_ptr<BatchAction>&& act) = 0;
  virtual void create_threads() = 0;
  virtual void start_working() = 0;
  virtual void init_threads() = 0;

  // TODO: 
  //    Add registration of ExecutionThreadManager. 
};

#endif // SCHEDULER_SYSTEM_H_
