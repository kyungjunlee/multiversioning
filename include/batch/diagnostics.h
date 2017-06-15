#ifndef BATCH_DIAGNOSTICS_H_
#define BATCH_DIAGNOSTICS_H_

#include "batch/print_util.h"
#include "batch/time_util.h"

// time the "time_what" chunk of code and invoke "invoke" on the resulting 
// duration in milliseconds. tp_name denotes the name of the timing variable
// for easy name collision evasion.
#define TIME_AND_INVOKE_ON_RESULT(time_what, invoke, tp_name)                  \
  do {                                                                         \
    TimeUtilities::TimePoint tp_name = TimeUtilities::now();                   \
    time_what                                                                  \
    invoke(TimeUtilities::time_difference_ms(tp_name, TimeUtilities::now()));  \
  } while (0)

#ifdef SCHEDULER_DIAG
#define TIME_IF_SCHED_DIAGNOSTICS(time_what, invoke, tp_name)                  \
  TIME_AND_INVOKE_ON_RESULT(time_what, invoke, tp_name)
#else
#define TIME_IF_SCHED_DIAGNOSTICS(time_what, invoke, tp_name)                  \
  do {                                                                         \
    time_what                                                                  \
  } while (0) 
#endif // SCHEDULER_DIAG

#ifdef SCHEDULER_DIAG 
#define IF_SCHED_DIAG(what)                                                    \
  what
#else
#define IF_SCHED_DIAG(what)                                                    \

#endif  // SCHEDULER_DIAG

#ifdef SCHEDULER_MAN_DIAG
#define TIME_IF_SCHED_MAN_DIAGNOSTICS(time_what, invoke, tp_name)              \
  TIME_AND_INVOKE_ON_RESULT(time_what, invoke, tp_name)
#else
#define TIME_IF_SCHED_MAN_DIAGNOSTICS(time_what, invoke, tp_name)              \
  do {                                                                         \
    time_what                                                                  \
  } while (0) 
#endif //SCHEDULER_MAN_DIAG

#ifdef SCHEDULER_DIAG 
#define IF_SCHED_MAN_DIAG(what)                                                \
  what
#else
#define IF_SCHED_MAN_DIAG(what)                                                \

#endif  // SCHEDULER_DIAG

class NumStat {
  private:
    void update_avg(double num);
    void update_min(double num);
    void update_max(double num);
    void update_samples();

  public:
    double average;
    double max;
    double min;
    unsigned int samples;

    NumStat();

    void add_sample(unsigned int num);
    void reset();
};

struct SchedulerDiag {
  NumStat time_per_iteration;
  NumStat time_creating_schedule;

  void reset();
};

struct GlobalSchedulerDiag {
  // all of the following will be collected from
  // SchedulerDiag for all scheduling threads in the system.
  NumStat avg_time_per_iteration;
  NumStat max_time_per_iteration;
  NumStat min_time_per_iteration;
  NumStat avg_num_of_iterations;

  NumStat avg_time_creating_schedule;
  NumStat max_time_creating_schedule;
  NumStat min_time_creating_schedule;

  void add_sample(const SchedulerDiag& sd);

  void print();
};

struct SchedulerManagerDiag {
  NumStat time_converting_workload;
  NumStat time_collecting_inputs;
  NumStat time_merging;
  NumStat time_signaling_no_destr;
  NumStat time_to_assign;
  NumStat time_to_create;

  SchedulerManagerDiag() {};
}; 

#endif // BATCH_DIAGNOSTICS_H_
