#ifndef TIME_TXN_ALLOCATION_H_
#define TIME_TXN_ALLOCATION_H_

#include "batch/txn_factory.h"
#include "batch/memory_pools.h"
#include "batch/RMW_batch_action.h"
#include "batch/time_util.h"
#include "batch/print_util.h"

#include <memory>

namespace TimeTxnAllocation {
  ActionSpecification get_action_spec() {
    LockDistributionConfig ldc = {
      .low_record = 0,
      .high_record = 999,
      .average_num_locks = 10,
      .std_dev_of_num_locks = 0
    };

    ActionSpecification as = {
      .writes = ldc,
      .reads = ldc
    };

    return as;
  };

  double time_individual_alloation(unsigned int number_of_txn) {
    ActionFactory<RMWBatchAction> act_factory(get_action_spec());
    auto time_it = [&act_factory, &number_of_txn](){
      act_factory.generate_actions(number_of_txn);
    };

    return TimeUtilities::time_function_ms(time_it);
  };

  double time_with_reuse(unsigned int number_of_txn, unsigned int alloc_num) {
    ActionFactory<RMWBatchAction> act_factory(get_action_spec());
    ActionMemoryPool<RMWBatchAction> amp; 
    std::vector<IBatchAction*> acts;
    auto& num_act_allocated = alloc_num < number_of_txn ? alloc_num : number_of_txn;
    acts.reserve(num_act_allocated);

    auto time_it = [&act_factory, number_of_txn, &acts, &amp, num_act_allocated]() {
      IBatchAction* curr_act;
      for (unsigned int i = 0; i < num_act_allocated; i++) {
        curr_act = amp.alloc_and_initialize();
        act_factory.initialize_txn_to_random_values(curr_act);
        acts.push_back(curr_act);
      }

      unsigned int index = 0;
      for (unsigned int i = 0; i < number_of_txn - num_act_allocated; i++) {
        index = i % num_act_allocated;
        curr_act = acts[index];
        amp.free((RMWBatchAction*) curr_act);

        curr_act = amp.alloc_and_initialize();
        act_factory.initialize_txn_to_random_values(curr_act);
        acts[index] = curr_act;
      }
    };

    auto result  = TimeUtilities::time_function_ms(time_it); 

    assert(acts.size() == num_act_allocated);
    for (auto& act : acts) {
      amp.free(act);
    }

    return result;
  };

  void time_txn_allocation() {
    TablePrinter tp;
    tp.set_table_header("Transaction Allocation and Initialization Time [ms]");
    tp.add_column_headers({
        "Experiment",
        "1 000 txns",
        "10 000 txns",
        "100 000 txns",
        "1 000 000 txns"});

    auto exec_and_add_to_table = [&tp](
        std::string row_name,
        std::function<double (unsigned int)> fun) {
      tp.add_row();
      tp.add_to_last_row(row_name);

      auto time_and_add = [&fun, &tp](unsigned int txns) {
        double result = 0;

        // repeat and average...
        for (unsigned int i = 0; i < 5; i++) {
          result += fun(txns);
        }

        result = result / 5;
        double per_fun_exec = result/txns;
        tp.add_to_last_row(
            PrintUtilities::double_to_string(result) +
              "(" + PrintUtilities::double_to_string(per_fun_exec) + ")"
        );
      }; 

      for (auto& txns : {1000, 10000, 100000, 1000000}) {
        time_and_add(txns);
      }
    };

    auto get_batch_fun_for = [](unsigned int alloc_num) {
      return [alloc_num](unsigned int txn_num) {
        return time_with_reuse(txn_num, alloc_num);
      };
    };

    exec_and_add_to_table("Always dynamic allocation", time_individual_alloation);
    for (auto& alloc_num : {10, 100, 1000, 10000}) {
      exec_and_add_to_table(
          "Allocate up to " + std::to_string(alloc_num) + " then reuse",
          get_batch_fun_for(alloc_num));
    } 

    tp.print_table();
  };
};

#endif //TIME_TXN_ALLOCATION_H_
