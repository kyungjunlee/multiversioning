#ifndef TIME_SPSC_QUEUE_H_
#define TIME_SPSC_QUEUE_H_

#include "batch/time_utilities.h"

#include <memory>

namespace TimeSpscQueue {
  SPSCMRQueue<unsigned int> get_prepped_queue(unsigned int elts) {
    SPSCMRQueue<unsigned int> q;  
    for (unsigned int i = 0; i < elts; i++) {
      q.push_tail(i);
    }

    return q;
  };

  double time_pop(unsigned int pop_reps) {
    auto q = get_prepped_queue(pop_reps);

    auto time_it = [&pop_reps, &q]() {
      for (unsigned int i = 0; i < pop_reps; i++) {
        q.pop_head();
      }
    };

   return TimeUtilities::time_function_ms(time_it); 
  };

  double time_peek_and_pop(unsigned int reps) {
    auto q = get_prepped_queue(reps);

    auto time_it = [&reps, &q]() {
      for (unsigned int i = 0; i < reps; i++) {
        std::shared_ptr<unsigned int> trash = q.peek_head();
        (void) trash;
        q.pop_head();
      }
    };

    return TimeUtilities::time_function_ms(time_it);
  };

  void time_queue() {
    TablePrinter tp;
    tp.set_table_header("SPSC queue timing [ms]");
    tp.add_column_headers({
       "Tested functions",
       "1000 reps",
       "10 000 reps",
       "1 000 000 reps",
       "10 000 000 reps"
    });
    
    auto exec_and_add_to_table = [&tp](
        std::string row_name,
        std::function<double (unsigned int)> fun) {
      tp.add_row();
      tp.add_to_last_row(row_name);

      auto time_and_add = [&fun, &tp](unsigned int reps) {
        double result = 0;
        for (unsigned int i = 0; i < 5; i++) {
          result += fun(reps);
        }
        result = result/5;

        double per_fun_exec = result/reps;
        tp.add_to_last_row(
            PrintUtilities::double_to_string(result) +
              "(" + PrintUtilities::double_to_string(per_fun_exec) + ")"
        );
      };

      for (auto& s : {1000, 10000, 1000000, 10000000}) {
        time_and_add(s);
      }
    };

    exec_and_add_to_table("pop", time_pop);
    exec_and_add_to_table("peak and pop", time_peek_and_pop);

    tp.print_table();
  };
};

#endif // TIME_SPSC_QUEUE_H_
