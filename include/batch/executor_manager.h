#ifndef EXECUTOR_MANAGER_H_
#define EXECUTOR_MANAGER_H_
#ifndef BATCH_EXECUTOR_H_
#define BATCH_EXECUTOR_H_

#include "batch/MS_queue.h"
#include "batch/batch_action.h"
#include "batch/scheduler.h"
#include "batch/executor.h"
#include "batch/scheduler_manager.h"

#include <list>
#include <memory>
#include <vector>


//  Executor Manager
//  
//    Executor manager is the data structure used to manage all of the 
//    execution threads in the system and the communication with them.
//
//    Executor manager class would also do all of the spin-off and 
//    destruction etc. It would be the point of communication of the 
//    execution threads with "outer world." This way we can keep such
//    communication relatively simple and orderly.

class ExecutorManager {
protected:
  std::vector<Executor> executors;
  SchedulerManager* scheduler_manager;
public:
  ExecutorManager();

  void initialize_executors(unsigned int num_threads);
  void register_schedulers(std::vector<Scheduler*> schedulers);
  void start_threads();
  void signal_threads(Scheduler* s);

  std::vector<Executor>* 
};


#endif //EXECUTOR_MANAGER_H_
