#ifndef SCHEDULER_THREAD_H_
#define SCHEDULER_THREAD_H_

#include "runnable.hh"
#include <stdint.h>

//  SchedulerThreadBatch
//
//      The general data structure containing a batch on which a scheduler
//      thread operates. Batches should be numbered in an increasing order.
struct SchedulerThreadBatch { 
  typedef Container::BatchActions BatchActions;

  BatchActions batch;
  uint64_t batch_id;
};

//  Scheduler Thread
//
//
//      The general interface of a scheduler thread required by the 
//      Scheduler Thread Manager. 
class SchedulerThreadManager;
class SchedulerThread : public Runnable {
  protected:
    SchedulerThreadManager* manager;
    uint64_t stop_signal;
    uint64_t thread_id;
    
    SchedulerThread(
        SchedulerThreadManager* manager,
        int m_cpu_number,
        uint64_t thread_id): 
      Runnable(m_cpu_number),
      manager(manager),
      stop_signal(false),
      thread_id(thread_id)
    {};

  public:
    using Runnable::StartWorking;
    using Runnable::Init;

    virtual uint64_t get_thread_id() { return thread_id; };
    virtual void signal_stop_working() = 0;
    virtual bool is_stop_requested() = 0;
 
    virtual ~SchedulerThread() {
      free(m_rand_state); 
    };
};

#endif // SCHEDULER_THREAD_H_
