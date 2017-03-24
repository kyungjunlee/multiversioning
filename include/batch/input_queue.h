#ifndef _GLOBAL_INPUT_QUEUE_H_
#define _GLOBAL_INPUT_QUEUE_H_

#include "batch/MS_queue.h"
#include "batch/batch_action_interface.h"
#include "batch/scheduler.h"

#include <vector>
#include <memory>
#include <cstdint>

class Scheduler;

/*
 *  Input Queue
 *
 *    Input Queue for all of the actions in the system. The actions are passed
 *    into the scheduling system through the SchedulingSystem class. 
 *    See include/batch/scheduler_system.h for the general interface. 
 */
class InputQueue : public MSQueue<std::unique_ptr<IBatchAction>> {
  private:
   using MSQueue<std::unique_ptr<IBatchAction>>::merge_queue;
};

#endif
