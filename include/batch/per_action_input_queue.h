#ifndef _GLOBAL_INPUT_QUEUE_H_
#define _GLOBAL_INPUT_QUEUE_H_

#include "batch/MS_queue.h"
#include "batch/batch_action_interface.h"
#include "batch/scheduler.h"
#include "batch/input_queue_interface.h"

#include <vector>
#include <memory>
#include <cstdint>

class Scheduler;

/*
 *  Per Action Input Queue
 *
 *    Implementation of the input queue for the system in which every action is
 *    stored separately and the batch is constructed on the fly as request for it
 *    comes in.
 */
class PerActionInputQueue : 
  private MSQueue<std::unique_ptr<IBatchAction>>,
  public InputQueue
{
public:
  PerActionInputQueue(uint32_t batch_size): 
    MSQueue<std::unique_ptr<IBatchAction>>(),
    InputQueue(batch_size)
  {};

  virtual InputQueue::BatchActions&& try_get_action_batch() override {
    // we return the whole batch only if the input queue is not empty. 
    // otherwise we return an empty vector!
    if (this->is_empty()) 
      return InputQueue::BatchActions();

    // Getting here means that we will be returning the WHOLE batch or
    // we will be waiting until we can return such.
    // TODO:
    //  Implement timeout.
    InputQueue::BatchActions batch(this->batch_size);
    for (unsigned int actionsTaken = 0; 
        actionsTaken < this->batch_size; 
        actionsTaken ++) {
      while (this->is_empty()) {};

      batch[actionsTaken] = std::move(this->peek_head());
      this->pop_head();
    }

    return std::move(batch);
  }; 

  virtual void add_action(std::unique_ptr<IBatchAction>&& act) override {
    MSQueue<std::unique_ptr<IBatchAction>>::push_tail(std::move(act));
  };

  virtual bool is_empty() override {
    return MSQueue<std::unique_ptr<IBatchAction>>::is_empty();
  };

  // flushing does not do anything since the queue is per-action 
  // any way. 
  virtual void flush() override {}
};

#endif
