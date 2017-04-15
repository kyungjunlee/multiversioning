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
 *    Implementation of the input queue for hte system in which actions
 *    are stored in batches and are returned on request.
 */
class BatchedInputQueue : 
  private MSQueue<std::vector<std::unique_ptr<IBatchAction>>>,
  public InputQueue
{
public:
  BatchedInputQueue(uint32_t batch_size): 
    MSQueue<std::vector<std::unique_ptr<IBatchAction>>>(),
    InputQueue(batch_size) {};

  virtual InputQueue::BatchActions try_get_action_batch() override {
    if (this->is_empty()) return InputQueue::BatchActions();

    auto return_value = std::move(this->peek_head());
    this->pop_head();

    return return_value;
  };

  virtual void add_action(std::unique_ptr<IBatchAction>&& act) override {
    // It is not necessary to create a new batch to put into the queue. 
    // This action will fit into the existing element.
    if (!this->is_empty() && this->peek_tail().size() < this->batch_size) {
      this->peek_tail().push_back(std::move(act));
      return;
    }

    // It is necessary to create a batch to put into the queue. 
    InputQueue::BatchActions batch;
    batch.push_back(std::move(act));
    this->push_tail(std::move(batch));
  };
};

#endif
