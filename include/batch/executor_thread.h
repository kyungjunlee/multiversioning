#ifndef EXECUTOR_THREAD_H_
#define EXECUTOR_THREAD_H_

#include "batch/batch_action_interface.h"
#include "runnable.hh"

#include <memory>
#include <vector>
#include <pthread.h>

class ExecutorThreadManager;
class ExecutorThread : public Runnable {
protected:
  ExecutorThreadManager* exec_manager;
  uint64_t stop_signal;

  ExecutorThread(
      ExecutorThreadManager* manager,
      int m_cpu_number):
    Runnable(m_cpu_number),
    exec_manager(manager),
    stop_signal(false)
  {};

public:
  using Runnable::StartWorking;
  using Runnable::Init;

  // TODO: 
  //    Make this typedef in global schedule so that it makes more sense? 
  //    Also... I think this will require changes in lock stages? Lock Stages
  //    don't own actions alone... they share them with the execution threads!
  typedef std::vector<std::shared_ptr<IBatchAction>> BatchActions;

  virtual void add_actions(BatchActions&& actions) = 0;
  virtual std::unique_ptr<BatchActions> try_get_done_batch() = 0;
  virtual void signal_stop_working() = 0;
  virtual bool is_stop_requested() = 0;

  virtual ~ExecutorThread() {
    free(m_rand_state);
  };
};

#endif // EXECUTOR_THREAD_H_
