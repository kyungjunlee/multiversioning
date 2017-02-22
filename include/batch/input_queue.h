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
 *    on to the scheduling threads in round-robbin manner. Notice that this means
 *    that the ordering of scheduling threads does not change (i+1 st thread will
 *    get a batch of actions after ith thread. Always.)
 */
class InputQueue : public MSQueue<std::unique_ptr<BatchAction>> {
  private:
    uint64_t holder;
    std::vector<Scheduler*> schedulers;
    using MSQueue<std::unique_ptr<BatchAction>>::merge_queue;
  public:
    InputQueue();
    void initialize(std::vector<Scheduler*> schedulers);
    // TODO:
    //    Implement a timeout. Right now we only base a batch on the number 
    //    of transactions. This is fine as long as we stress-test the system.
    virtual void obtain_batch(Scheduler* s);
};

#endif
