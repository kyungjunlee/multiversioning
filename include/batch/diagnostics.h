#ifndef BATCH_DIAGNOSTICS_H_
#define BATCH_DIAGNOSTICS_H_

#include "batch/print_util.h"

#ifdef SCHEDULER_DIAG
// if diagnostics requested, time the "time_what" chunk of code and 
// invoke "invoke" on the resulting duration in milliseconds. tp_name denotes
// the name of the timing variables for easy name collision evasion.
#define TIME_IF_SCHED_DIAGNOSTICS(time_what, invoke, tp_name)                  \
  do {                                                                         \
    TimeUtilities::TimePoint tp_name = TimeUtilities::now();                   \
    time_what                                                                  \
    invoke(TimeUtilities::time_difference_ms(tp_name, TimeUtilities::now()));  \
  } while (0)
#else
// If diagnostics are not requested, just do time_what.
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
  
#endif //SCHEDULER_DIAG

class NumStat {
  private:
    void update_avg(double num) {
      average /= samples + 1;
      average *= samples;
      average += num / (samples + 1);
    };
    
    void update_min(double num) {
      min = (min > num || samples == 0) ? num : min;
    };
    
    void update_max(double num) {
      max = (max < num || samples == 0) ? num : max;
    };
    
    void update_samples() {
      samples ++;
    };

  public:
    double average;
    double max;
    double min;
    unsigned int samples;

    NumStat():
      average(0),
      max(0),
      min(0),
      samples(0)
    {};

    void add_sample(unsigned int num) {
      update_avg(num);
      update_min(num);
      update_max(num);
      update_samples();
    };

    void reset() {
      average = 0;
      max = 0;
      min = 0;
      samples = 0;
    }
};

struct SchedulerDiag {
  NumStat time_per_iteration;
  NumStat time_creating_schedule;

  void reset() {
    time_per_iteration.reset();
    time_creating_schedule.reset();
  };
};

struct GlobalSchedulerDiag {
  // all of the following will be collected from
  // SchedulerDiag for all scheduling threads in the system.
  NumStat avg_time_per_iteration;
  NumStat max_time_per_iteration;
  NumStat min_time_per_iteration;
  NumStat avg_time_creating_schedule;
  NumStat max_time_creating_schedule;
  NumStat min_time_creating_schedule;

  void add_sample(const SchedulerDiag& sd) {
    auto& tpi = sd.time_per_iteration;
    auto& tcs = sd.time_creating_schedule;
    
    avg_time_per_iteration.add_sample(tpi.average);
    max_time_per_iteration.add_sample(tpi.max);
    min_time_per_iteration.add_sample(tpi.min);

    avg_time_creating_schedule.add_sample(tcs.average);
    max_time_creating_schedule.add_sample(tcs.max);
    min_time_creating_schedule.add_sample(tcs.min);
  };

  void print() {
    TablePrinter tp;
    tp.set_table_header("Scheduler Thread Diagnostics");
    tp.add_column_headers({
      "min",
      "max",
      "avg",
      "data count"
    });

    auto add_formatted_row = [&tp](std::string row_name, NumStat stat) {
      tp.add_row();
      tp.add_to_last_row({
          row_name,
          PrintUtilities::double_to_string(stat.min),
          PrintUtilities::double_to_string(stat.max),
          PrintUtilities::double_to_string(stat.average),
          std::to_string(stat.samples)
      });
    };

    add_formatted_row("Average time per iteration", avg_time_per_iteration);
    add_formatted_row("Max time per iteration", max_time_per_iteration);
    add_formatted_row("Min time per iteration", min_time_per_iteration);

    add_formatted_row("Average time creating schedule", avg_time_creating_schedule);
    add_formatted_row("Max time creating schedule", max_time_creating_schedule);
    add_formatted_row("Min time creating schedule", min_time_creating_schedule);

    tp.print_table();
  };
};

#endif // BATCH_DIAGNOSTICS_H_
