#ifndef SUPERVISOR_H_
#define SUPERVISOR_H_

#include "batch/supervisor_interface.h"

class Supervisor : public SupervisorInterface {
public:
  Supervisor(
      SchedulingSystemConfig sched_conf,
      ExecutingSystemConfig exec_conf);

  GlobalSchedule gs;
  SchedulerManager sched;
  ExecutorManager exec;
};

#endif // SUPERVISOR_H_
