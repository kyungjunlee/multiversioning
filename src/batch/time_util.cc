#include "batch/time_util.h"

#include <functional>

template <typename TimeUnit> 
double TimeUtilities::time_difference(TimePoint start, TimePoint end) {
  return duration_cast<TimeUnit>(end - start).count();
};

double TimeUtilities::time_difference_ms(TimePoint start, TimePoint end) {
  return time_difference<milliseconds>(start, end);
};

double TimeUtilities::time_function_ms(std::function<void (void)> fun) {
  TimePoint start = system_clock::now();
  fun();
  return time_difference_ms(start, system_clock::now()); 
}

TimeUtilities::TimePoint TimeUtilities::now() { 
  return system_clock::now(); 
}

