#ifndef TIME_UTILITIES_H
#define TIME_UTILITIES_H

#include "batch/SPSC_MR_queue.h"

#include <chrono>
#include <functional>

namespace TimeUtilities {
  using namespace std::chrono;
  typedef system_clock::time_point TimePoint;

  template <typename TimeUnit> 
  double time_difference(TimePoint start, TimePoint end) {
    return duration_cast<TimeUnit>(end - start).count();
  };

  double time_difference_ms(TimePoint start, TimePoint end) {
    return time_difference<milliseconds>(start, end);
  };

  double time_function_ms(std::function<void (void)> fun) {
    TimePoint start = system_clock::now();
    fun();
    return time_difference_ms(start, system_clock::now()); 
  }
};

#endif //TIME_UTILITIES_H
