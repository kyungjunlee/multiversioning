#include "batch/supervisor.h"

Supervisor::Supervisor(
    DBStorageConfig db_conf,
    SchedulingSystemConfig sched_conf,
    ExecutingSystemConfig exec_conf):
  ISupervisor(sched_conf, exec_conf),
  db(db_conf),
  gs(db_conf),
  exec(exec_conf),
  sched(sched_conf, &this->exec)
{
  this->sched_system = &sched;
  this->exec_system = &exec;

  // initialize exec system
  exec.set_global_schedule_ptr(&gs);
  exec.set_db_storage_ptr(&db);

  // initialize sched system
  sched.set_global_schedule_ptr(&gs);
};
