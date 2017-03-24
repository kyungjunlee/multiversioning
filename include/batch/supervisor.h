#ifndef SUPERVISOR_H_
#define SUPERVISOR_H_

#include "batch/supervisor_interface.h"
#include "batch/scheduler_manager.h"
#include "batch/executor_manager.h"
#include "batch/global_schedule.h"

class Supervisor : public SupervisorInterface {
public:
  Supervisor(
      SchedulingSystemConfig sched_conf,
      ExecutingSystemConfig exec_conf);

  GlobalSchedule gs;
  ExecutorManager exec;
  SchedulerManager sched;
};

#endif // SUPERVISOR_H_
