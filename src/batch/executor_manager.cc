#include "batch/executor_manager.h"

ExecutorManager::ExecutorManager(ExecutingSystemConfig conf):
  ExecutingSystem(conf),
  next_signaled_executor(0),
  next_output_executor(0)
{
  create_threads();
};

void ExecutorManager::create_threads() {
  for (unsigned int i = 0; i < conf.executing_threads_count; i++) {
    executors.push_back(
        std::make_shared<BatchExecutor>(this, conf.first_pin_cpu_id + i));
  }
};

void ExecutorManager::start_working() {
  assert(gs != nullptr);
  for (auto executor : executors) {
    executor->Run();
    executor->WaitInit();
  }
};

void ExecutorManager::init() {
  for (auto executor : executors) {
    executor->Init();
  }
}

void ExecutorManager::stop_working() {
  for (auto executor : executors) {
    executor->signal_stop_working();
    executor->Join();
  }
};

unsigned int ExecutorManager::get_executor_num() {
  return executors.size();
}

void ExecutorManager::signal_execution_threads(
    ExecutorThreadManager::ThreadWorkloads&& workloads) {
  for (auto workload : workloads) {
    executors[next_signaled_executor]->add_actions(std::move(workload));  
    next_signaled_executor ++;
    next_signaled_executor %= executors.size();
  } 
};

std::unique_ptr<ExecutorThread::BatchActions> 
ExecutorManager::get_done_batch() {
  std::unique_ptr<ExecutorThread::BatchActions> batch;
  while ((batch = try_get_done_batch()) == nullptr);

  return std::move(batch);  
}

std::unique_ptr<ExecutorThread::BatchActions> 
ExecutorManager::try_get_done_batch() {
  auto batch = executors[next_output_executor]->try_get_done_batch();
  next_output_executor ++;
  next_output_executor %= executors.size();

  return std::move(batch);
}

void ExecutorManager::set_global_schedule_ptr(IGlobalSchedule* gs) {
  this->gs = gs;
}

void ExecutorManager::set_db_storage_ptr(IDBStorage* db) {
  this->db = db;
}

std::shared_ptr<LockStage> 
ExecutorManager::get_current_lock_holder_for(RecordKey key) {
  return gs->get_stage_holding_lock_for(key);
}

void ExecutorManager::finalize_action(std::shared_ptr<IBatchAction> act) {
  gs->finalize_execution_of_action(act);
}

ExecutorManager::~ExecutorManager() {
  // just to be safe
  stop_working();
}
