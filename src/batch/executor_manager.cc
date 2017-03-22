#include "batch/executor_manager.h"

ExecutorManager::ExecutorManager(ExecutingSystemConfig conf):
  ExecutingSystem(conf),
  next_signaled_executor(0),
  next_output_executor(0)
{};

void ExecutorManager::initialize_executors() {
  // TODO:
  //    Create the threads.
};

void ExecutorManager::start_working() {
  for (auto executor : executors) {
    executor->StartWorking();
  }
};

void ExecutorManager::init_threads() {
  for (auto executor : executors) {
    executor->Init();
  }
}

void ExecutorManager::signal_execution_threads(
    std::vector<ExecutorThread::BatchActions> workloads) {
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
