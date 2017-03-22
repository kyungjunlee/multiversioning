#ifndef BATCH_EXECUTOR_H_
#define BATCH_EXECUTOR_H_

#include "batch/MS_queue.h"
#include "batch/batch_action.h"

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
//
//    The output queue of an executor is used only by the executor itself and
//    by the simulation framework.
class ExecutorQueue : public MSQueue<std::vector<BatchAction>> {
private:
  using MSQueue<ExecutorThread::BatchActions:>::merge_queue;
};

// Pending Queue
//    
//    Pending Queue is used by the executor to keep track of actions within
//    a particular batch that could not be executed when the executor attempted
//    to do so.
typedef std::list<std::shared_ptr<BatchAction>> PendingQueue;

class Executor : public ExecutorThread {
protected:
  std::unique_ptr<ExecutorQueue> input_queue;
  std::unique_ptr<ExecutorQueue> output_queue;
  // Pending actions are those that may not be immediately executed, but 
  // belong to the currently processed batch.
  std::unique_ptr<PendingQueue> pending_queue;
  std::unique_ptr<ExecutorThread::BatchActions> currentBatch;

  void process_action_batch();
  // true if successful and false otherwise
  bool process_action(BatchAction* act);
  void process_blocking_actions(BatchAction* act);
  void process_pending();
  
public:
  Executor(
      ExecutorThreadManager* manager,
      int m_cpu_number);

  // override of Runnable interface
  void StartWorking() override;
  void Init() override();
  
  // implement the eecutor thread interface.
  void add_actions(ExecutorThread::BatchActions&& actions);
  std::shared_ptr<BatchAction> try_get_done_action();
};

#endif //BATCH_EXECUTOR_H_
