#ifndef TEST_INPUT_QUEUE_H_
#define TEST_INPUT_QUEUE_H_

#include "batch/input_queue.h"
#include "batch/scheduler.h"

class TestInputQueue : public InputQueue {
public:
  void obtain_batch (Scheduler* s) override {
    (void) s;
  };
};
#endif
