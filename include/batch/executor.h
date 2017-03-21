#ifndef BATCH_EXECUTOR_H_
#define BATCH_EXECUTOR_H_

#include "batch/MS_queue.h"
#include "batch/batch_action.h"

#include <list>
#include <memory>
#include <vector>

// TODO:
//  Executor manager class that is not a thread, but rather is used 
//  for communication between scheduling threads and execution
//  threads. Should do all the logical locking etc.
//
//  Executor manager class would also do all of the spin-off and 
//  destruction etc. It would be the point of communication of the 
//  execution threads with "outer world." This way we can keep such
//  communication relatively simple and orderly.

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
  using MSQueue<std::vector<BatchAction>>::merge_queue;
};

struct ExecutorConfig {
  std::unique_ptr<ExecutorQueue> input_queue;
  std::unique_ptr<ExecutorQueue> output_queue;
  // TODO:
  //    Pointer to the global schedule.
  // std::shared_ptr<GlobalSchedule> global_schedule;
  int m_cpu_number; 
};

// Pending Queue
//    
//    Pending Queue is used by the executor to keep track of actions within
//    a particular batch that could not be executed when the executor attempted
//    to do so.
//
//    The BatchAction* pointers are just observing pointers. The currentBatch
//    element within executor still owns the actions since the simulation framework
//    will want to get those.
typedef std::list<BatchAction*> PendingQueue;

class Executor : public Runnable {
protected:
  std::unique_ptr<ExecutorQueue> input_queue;
  std::unique_ptr<ExecutorQueue> output_queue;
  // Pending actions are those that may not be immediately executed, but 
  // belong to the currently processed batch.
  std::unique_ptr<PendingQueue> pending_queue;
  // TODO:
  //      std::shared_ptr<GlobalSchedule> global_schedule;
  std::unique_ptr<std::vector<BatchAction>> currentBatch;

  void process_action_batch();
  // true if successful and false otherwise
  bool process_action(BatchAction* act);
  void process_blocking_actions(BatchAction* act);
  void process_pending();
  
public:
  Executor(Executorconfig cfg);

  // override of Runnable interface
  void StartWorking() override;
  void Init() override();
  
  // own functions
  void enqueue_batch(std::vector<BatchAction>&& batch);
};

#endif //BATCH_EXECUTOR_H_
