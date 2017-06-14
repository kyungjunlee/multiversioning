#ifndef TIME_UTILITIES_H_
#define TIME_UTILITIES_H_

#include "batch/SPSC_MR_queue.h"

#include <chrono>

namespace TimeUtilities {
  using namespace std::chrono;
  typedef system_clock::time_point TimePoint;

  template <typename TimeUnit> 
  double time_difference(TimePoint start, TimePoint end);

  double time_difference_ms(TimePoint start, TimePoint end);

  double time_function_ms(std::function<void (void)> fun);

  TimePoint now();
};

#endif //TIME_UTILITIES_H_
