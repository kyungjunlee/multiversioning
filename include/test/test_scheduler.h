#ifndef TEST_SCHEDULER_H_
#define TEST_SCHEDULER_H_

#include "batch/scheduler.h"

class TestScheduler : public Scheduler {
public:
  TestScheduler(SchedulerConfig cf): Scheduler(cf) {};
  std::vector<std::unique_ptr<BatchAction>> putActions;
  
  void putAction(std::unique_ptr<BatchAction> act) override {
    putActions.push_back(std::move(act));
  };
};

#endif // TEST_SCHEDULER_H_
