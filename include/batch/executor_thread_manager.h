#ifndef EXECUTOR_THREAD_MANAGER_H_
#define EXECUTOR_THREAD_MANAGER_H_

#include "batch/executor_thread.h"
#include "batch/batch_action_interface.h"
#include "batch/lock_stage.h"
#include "batch/record_key.h"

#include <memory>
#include <vector>

class ExecutorThreadManager {
  public:
    typedef std::vector<ExecutorThread::BatchActions> SignalWorkload;

    virtual void signal_execution_threads(SignalWorkload&& workload) = 0;
    virtual std::shared_ptr<LockStage>
      get_current_lock_holder_for(RecordKey key) = 0;
    virtual void finalize_action(std::shared_ptr<BatchActionInterface> act) = 0;
};

#endif // EXECUTOR_THREAD_MANAGER_H_