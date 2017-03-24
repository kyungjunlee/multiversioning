#ifndef EXECUTOR_SYSTEM_H_
#define EXECUTOR_SYSTEM_H_

#include "batch/global_schedule.h"
#include "batch/executor_thread.h"

#include <memory>

// Executing System Config
//  
//    Used to configure the executing system within the system.
struct ExecutingSystemConfig {
  uint32_t executing_threads_count;
};

// Executing System
//   
//    Represents the exeucting system. This is what the supervisor class
//    thinks the executing system is. The only interactions between the 
//    supervisor and the scheduling system are described by this interface.
class ExecutingSystem {
protected:
  ExecutingSystemConfig conf;
public:
  ExecutingSystem(ExecutingSystemConfig c): conf(c) {};

  virtual std::unique_ptr<ExecutorThread::BatchActions> get_done_batch() = 0;  
  virtual std::unique_ptr<ExecutorThread::BatchActions> try_get_done_batch() = 0;  
  virtual void set_global_schedule_ptr(GlobalScheduleInterface* gs) = 0;
  virtual void start_working() = 0;
  virtual void init() = 0;
};

#endif // EXECUTOR_SYSTEM_H_
