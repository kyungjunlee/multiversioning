#ifndef EXECUTOR_MANAGER_H_
#define EXECUTOR_MANAGER_H_
#ifndef BATCH_EXECUTOR_H_
#define BATCH_EXECUTOR_H_

#include "batch/batch_action.h"
#include "batch/scheduler.h"
#include "batch/executor.h"
#include "batch/scheduler_manager.h"

#include <list>
#include <memory>
#include <vector>


//  Executor Manager
//  
//  Executor Manager is the actual implementaiton of Executing System and
//  Executor Thread Manager classes. The supervisor class has a handle to it
//  using a Executing System pointer and the executing threads have a handle
//  to it using the ExecutorThreadManager pointer.
class ExecutorManager :
  public ExecutorThreadManager,
  public ExecutingSystem {
public:
  std::vector<std::shared_ptr<ExecutorThread>> executors;
  unsigned int next_signaled_executor;
  unsigned int next_output_executor;

  ExecutorManager();

  // implementing the ExecutingSystem interface
  virtual std::unique_ptr<BatchAction> get_done_action();
  virtual std::unique_ptr<BatchAction> try_get_done_action();
  virtual void start_working();
  virtual void init_threads();

  // implementing the ExecutorThreadManager interface
  virtual void signal_execution_threads(
      std::vector<ExecutorThread::BatchActions> workload);
};


#endif //EXECUTOR_MANAGER_H_
