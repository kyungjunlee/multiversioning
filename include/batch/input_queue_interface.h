#ifndef INPUT_QUEUE_INTERFACE_H_
#define INPUT_QUEUE_INTERFACE_H_

#include "batch/batch_action_interface.h"

#include <memory>
#include <vector>

/*
 *  InputQueue
 *
 *    The interface for the input queue for all of the actions in the system.
 *    The actions are passed into the scheduling system through the SchedulingSystem
 *    class. Please see include/batch/scheduler_system.h for the general interface.
 *
 *    NOTE:
 *      This is a thread-safe one-producer, one-consumer implementation!
 */
class InputQueue {
protected:
  uint32_t batch_size;
public:
  InputQueue(uint32_t bs): batch_size(bs) {};
  typedef std::vector<std::unique_ptr<IBatchAction>> BatchActions;
  
  // Try to get a batch of actions. Returns an empty vector if no actions
  // are available and otherwise busy waits until a whole batch is available.
  virtual BatchActions try_get_action_batch() = 0;

  // Puts the action into the global queue. Does not guarantee that the actions
  // are visible to the execution threads. To enforce "flushing" consider calling
  // the flush function. 
  virtual void add_action(std::unique_ptr<IBatchAction>&& act) = 0;

  // Flushes all of the actions added so that they are visible by the execution
  // threads. Notice that this is necessary if the implementation of input queue
  // performs any form of batching.
  virtual void flush() = 0;

  virtual ~InputQueue() {};
};

#endif // INPUT_QUEUE_INTERFACE_H_
