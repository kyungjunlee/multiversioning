#include "batch/diagnostics.h"

void NumStat::update_avg(double num) {
  average /= samples + 1;
  average *= samples;
  average += num / (samples + 1);
};

void NumStat::update_min(double num) {
  min = (min > num || samples == 0) ? num : min;
};

void NumStat::update_max(double num) {
  max = (max < num || samples == 0) ? num : max;
};

void NumStat::update_samples() {
  samples ++;
};

NumStat::NumStat():
  average(0),
  max(0),
  min(0),
  samples(0)
{};

void NumStat::add_sample(unsigned int num) {
  update_avg(num);
  update_min(num);
  update_max(num);
  update_samples();
};

void NumStat::reset() {
  average = 0;
  max = 0;
  min = 0;
  samples = 0;
};

void SchedulerDiag::reset() {
  time_per_iteration.reset();
  time_creating_schedule.reset();
};

#define HANDLE_AVERAGING_STATS(core_new_stat_name, old_stat_name)              \
  avg_ ## core_new_stat_name.add_sample(old_stat_name.average);                \
  max_ ## core_new_stat_name.add_sample(old_stat_name.max);                    \
  min_ ## core_new_stat_name.add_sample(old_stat_name.min);                    

void GlobalSchedulerDiag::add_sample(const SchedulerDiag& sd) {
  auto& tpi = sd.time_per_iteration;
  auto& tcs = sd.time_creating_schedule;
  
  HANDLE_AVERAGING_STATS(time_per_iteration, tpi);
  HANDLE_AVERAGING_STATS(time_creating_schedule, tcs);
  avg_num_of_iterations.add_sample(tpi.samples);
};

#define APPLY_TO_ALL_STATS(fun_name, core_string, core_stat)                   \
  fun_name("Average " #core_string, avg_ ## core_stat);                        \
  fun_name("Max " #core_string, max_ ## core_stat);                            \
  fun_name("Min " #core_string, min_ ## core_stat);                           

void GlobalSchedulerDiag::print() {
  TablePrinter tp;
  tp.set_table_header("Scheduler Threads Diagnostics");
  tp.add_column_headers({
    "Measurements",
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

  APPLY_TO_ALL_STATS(add_formatted_row, "time per iteration", time_per_iteration);
  add_formatted_row("Avg number of iterations", avg_num_of_iterations);

  APPLY_TO_ALL_STATS(
      add_formatted_row, "time creating schedule", time_creating_schedule);

  tp.print_table();
};

