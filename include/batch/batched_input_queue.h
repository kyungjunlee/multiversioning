#ifndef BATCHED_GLOBAL_INPUT_QUEUE_H_
#define BATCHED_GLOBAL_INPUT_QUEUE_H_

#include "batch/input_queue_interface.h"
#include "batch/MS_queue.h"
#include "batch/batch_action_interface.h"
#include "batch/scheduler.h"

#include <vector>
#include <memory>
#include <cstdint>

class Scheduler;

/*
 *  Batched Input Queue
 *    
 *    Implementation of the input queue for the system in which actions
 *    are stored in batches and are returned on request.
 */
class BatchedInputQueue : 
  private MSQueue<std::vector<std::unique_ptr<IBatchAction>>>,
  public InputQueue
{
private:
  // private container for the batch before it is visible to the
  // execution threads. Necessary to make sure that adding transactions
  // is thread safe!
  std::vector<std::unique_ptr<IBatchAction>> currentBatch;

public:
  BatchedInputQueue(uint32_t batch_size): 
    MSQueue<std::vector<std::unique_ptr<IBatchAction>>>(),
    InputQueue(batch_size) {};

  virtual InputQueue::BatchActions try_get_action_batch() override {
    if (MSQueue<std::vector<std::unique_ptr<IBatchAction>>>::is_empty()) 
      return InputQueue::BatchActions();

    auto return_value = std::move(this->peek_head());
    this->pop_head();

    return return_value;
  };

  virtual void add_action(std::unique_ptr<IBatchAction>&& act) override {
    assert(this->batch_size > 0);
    // It is not necessary to create a new batch to put into the queue. 
    // This action will fit into the existing element.
    if (this->currentBatch.size() < this->batch_size - 1) {
      currentBatch.push_back(std::move(act));
      return;
    }

    // push the batch into queue and prepare the container for reuse.
    this->currentBatch.push_back(std::move(act));
    this->flush();
  };

  virtual bool is_empty() override {
    return MSQueue<std::vector<std::unique_ptr<IBatchAction>>>::is_empty();
  };

  virtual void flush() override {
    this->push_tail(std::move(currentBatch));
    this->currentBatch.clear();
  };
};

#endif
