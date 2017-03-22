#ifndef _GLOBAL_INPUT_QUEUE_H_
#define _GLOBAL_INPUT_QUEUE_H_

#include "batch/MS_queue.h"
#include "batch/batch_action.h"
#include "batch/scheduler.h"

#include <vector>
#include <memory>
#include <cstdint>

class Scheduler;

/*
 *  Input Queue
 *
 *    Input Queue for all of the actions in the system. The actions are passed
 *    on to the scheduling threads in round-robbin manner by the SchedulerThreadManager.
 *    See include/batch/scheduler_manager.h for more information. 
 */
class InputQueue : public MSQueue<std::unique_ptr<BatchAction>> {
  private:
    using MSQueue<std::unique_ptr<BatchAction>>::merge_queue;
};

#endif
