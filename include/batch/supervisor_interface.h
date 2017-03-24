#ifndef SUPERVISOR_INTERFACE_H_
#define SUPERVISOR_INTERFACE_H_

#include "batch/scheduler_system.h"
#include "batch/executor_system.h"
#include "batch/batch_action_interface.h"

class SupervisorInterface {
protected:
  SchedulingSystemConfig sched_conf;
  ExecutingSystemConfig exec_conf;

  SchedulingSystem* sched_system;
  ExecutingSystem* exec_system;
public:
  SupervisorInterface(
      SchedulingSystemConfig sched_conf,
      ExecutingSystemConfig exec_conf):
    sched_conf(sched_conf),
    exec_conf(exec_conf)
  {};

  typedef std::vector<std::unique_ptr<BatchActionInterface>> SimulationWorkload;
  virtual void set_simulation_workload(
      std::vector<std::unique_ptr<BatchActionInterface>>&& workload) {
    for(auto& act_ptr : workload) {
      sched_system->add_action(std::move(act_ptr));
    }
  };

  virtual std::unique_ptr<std::vector<std::shared_ptr<BatchActionInterface>>> 
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
// TODO:
//  virtual void stop_system() = 0;
};

#endif //SUPERVISOR_INTERFACE_H_
