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

  // Puts the action into the global queue.
  virtual void add_action(std::unique_ptr<IBatchAction>&& act) = 0;

  virtual ~InputQueue() {};
};

#endif // INPUT_QUEUE_INTERFACE_H_
