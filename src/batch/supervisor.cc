#include "batch/supervisor.h"

Supervisor::Supervisor(
    SchedulingSystemConfig sched_conf,
    ExecutingSystemConfig exec_conf):
  SupervisorInterface(sched_conf, exec_conf),
  gs(),
  exec(exec_conf),
  sched(sched_conf, &this->exec)
{
  this->sched_system = &sched;
  this->exec_system = &exec;

  // set up the global schedule
  exec.set_global_schedule_ptr(&gs);
  sched.set_global_schedule_ptr(&gs);
};
