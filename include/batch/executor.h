#ifndef BATCH_EXECUTOR_H_
#define BATCH_EXECUTOR_H_

#include "batch/MS_queue.h"
#include "batch/batch_action_interface.h"
#include "batch/executor_thread.h"
#include "batch/executor_thread_manager.h"

#include <list>
#include <memory>
#include <vector>

// ExecutorQueue
//    
//    Executor queue is a single-producer, single-consumer ms-queue. Every
//    executor owns two executor queues -- one for input and one for output.
//    The executor manager contains handles to all the executor threads
//    and may be used by scheduling threads to assign ownership of actions
//    to execution threads in a synchronized manner. 
class ExecutorQueue : public MSQueue<std::unique_ptr<ExecutorThread::BatchActions>> {
private:
  using MSQueue<std::unique_ptr<ExecutorThread::BatchActions>>::merge_queue;
};

// Pending Queue
//    
//    Pending Queue is used by the executor to keep track of actions within
//    a particular batch that could not be executed when the executor attempted
//    to do so.
typedef std::list<std::shared_ptr<BatchActionInterface>> PendingList;

// BatchExecutor
//
//    The actual implementation of ExecutorThread used within the system. This is
//    the primary worker in the system which executes actions that are assigned to
//    it.
class BatchExecutor : public ExecutorThread {
protected:
  std::unique_ptr<ExecutorQueue> input_queue;
  std::unique_ptr<ExecutorQueue> output_queue;
  // Pending actions are those that may not be immediately executed, but 
  // belong to the currently processed batch.
  std::unique_ptr<PendingList> pending_list;
  std::unique_ptr<ExecutorThread::BatchActions> currentBatch;

  void process_action_batch();
  // true if successful and false otherwise
  bool process_action(std::shared_ptr<BatchActionInterface> act);
  void process_pending();
  
public:
  BatchExecutor(
      ExecutorThreadManager* manager,
      int m_cpu_number);

  // implement the Runnable interface
  void StartWorking() override;
  void Init() override;
  
  // implement the executor thread interface.
  void add_actions(ExecutorThread::BatchActions&& actions) override;
  std::unique_ptr<ExecutorThread::BatchActions> try_get_done_batch() override;
};

#endif //BATCH_EXECUTOR_H_
