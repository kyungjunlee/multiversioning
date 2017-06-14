#ifndef BATCH_DIAGNOSTICS_H_
#define BATCH_DIAGNOSTICS_H_

#ifdef SCHEDULER_DIAG
#define TIME_IF_DIAGNOSTICS(time_what, invoke, tp_name)                        \
  do {                                                                         \
    TimeUtilities::TimePoint tp_name = TimeUtilities::now();                   \
    time_what                                                                  \
    invoke(TimeUtilities::time_difference_ms(tp_name, TimeUtilities::now()));  \
  } while (0)
#else
// If diagnostics are not requested, do nothing.
#define TIME_IF_DIAGNOSTICS(time_what, invoke, tp_name)                        \
  do {                                                                         \
    time_what                                                                  \
  } while (0) 
#endif // SCHEDULER_DIAG

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
};

struct SchedulerDiag {
  NumStat time_per_iteration;
  NumStat time_creating_schedule; 
};

#endif // BATCH_DIAGNOSTICS_H_
