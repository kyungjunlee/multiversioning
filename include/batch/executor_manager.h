#ifndef EXECUTOR_MANAGER_H_
#define EXECUTOR_MANAGER_H_

#include "batch/batch_action_interface.h"
#include "batch/record_key.h"
#include "batch/scheduler.h"
#include "batch/executor.h"
#include "batch/executor_thread_manager.h"
#include "batch/executor_system.h"

#include <list>
#include <memory>
#include <vector>

//  Executor Manager
//  
//  Executor Manager is the actual implementation of the Executing System and
//  Executor Thread Manager classes. The supervisor class has a handle to it
//  using a Executing System pointer and the executing threads have a handle
//  to it using the ExecutorThreadManager pointer.
class ExecutorManager :
  public ExecutingSystem,
  public ExecutorThreadManager {
private:
  void create_threads();
public:
  std::vector<std::shared_ptr<ExecutorThread>> executors;
  unsigned int next_signaled_executor;
  unsigned int next_output_executor;
  IGlobalSchedule* gs;
  IDBStorage* db;

  ExecutorManager(ExecutingSystemConfig c);

  // implementing the ExecutingSystem interface
  virtual std::unique_ptr<ExecutorThread::BatchActions> get_done_batch() override;
  virtual std::unique_ptr<ExecutorThread::BatchActions> try_get_done_batch() override;
  virtual void set_global_schedule_ptr(IGlobalSchedule* gs) override;
  virtual void set_db_storage_ptr(IDBStorage* db) override;
  virtual void start_working() override;
  virtual void init() override;
  virtual void stop_working() override;

  // implementing the ExecutorThreadManager interface
  virtual unsigned int get_executor_num() override;
  virtual void signal_execution_threads(
      ExecutorThreadManager::ThreadWorkloads&& workload) override;
  virtual std::shared_ptr<LockStage> 
    get_current_lock_holder_for(RecordKey key) override;
  virtual void finalize_action(std::shared_ptr<IBatchAction> act) override;

  virtual ~ExecutorManager();
};

#endif //EXECUTOR_MANAGER_H_
