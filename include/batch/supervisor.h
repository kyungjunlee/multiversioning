#ifndef SUPERVISOR_H_
#define SUPERVISOR_H_

#include "batch/supervisor_interface.h"
#include "batch/scheduler_manager.h"
#include "batch/executor_manager.h"
#include "batch/global_schedule.h"
#include "batch/db_storage.h"

class Supervisor : public ISupervisor {
public:
  Supervisor(
      DBStorageConfig db_conf,
      SchedulingSystemConfig sched_conf,
      ExecutingSystemConfig exec_conf);

  DBStorage db;
  GlobalSchedule gs;
  ExecutorManager exec;
  SchedulerManager sched;

  virtual IDBStorage* get_db_pter() override;
};

#endif // SUPERVISOR_H_
