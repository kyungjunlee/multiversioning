#ifndef SUPERVISOR_INTERFACE_H_
#define SUPERVISOR_INTERFACE_H_

#include "batch/db_storage_interface.h"
#include "batch/scheduler_system.h"
#include "batch/executor_system.h"
#include "batch/batch_action_interface.h"

class ISupervisor {
protected:
  DBStorageConfig db_conf;
  SchedulingSystemConfig sched_conf;
  ExecutingSystemConfig exec_conf;

  SchedulingSystem* sched_system;
  ExecutingSystem* exec_system;
public:
  ISupervisor(
      SchedulingSystemConfig sched_conf,
      ExecutingSystemConfig exec_conf):
    sched_conf(sched_conf),
    exec_conf(exec_conf)
  {};

  typedef std::vector<std::unique_ptr<IBatchAction>> SimulationWorkload;
  virtual void set_simulation_workload(
      std::vector<std::unique_ptr<IBatchAction>>&& workload) {
    for(auto& act_ptr : workload) {
      sched_system->add_action(std::move(act_ptr));
    }
  };

  virtual std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>> 
    get_output() {
      return std::move(exec_system->try_get_done_batch());
  };
  
  virtual void init_system() {
    sched_system->init();
    exec_system->init(); 
  };

  virtual void start_system() {
    sched_system->start_working();
    exec_system->start_working(); 
  };
  
  virtual void stop_system() {
    exec_system->stop_working(); 
    sched_system->stop_working();
  };

  virtual IDBStorage* get_db_pter() = 0;

  virtual ~ISupervisor() {};
};

#endif //SUPERVISOR_INTERFACE_H_
