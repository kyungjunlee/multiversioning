#ifndef TIME_TXN_ALLOCATION_H_
#define TIME_TXN_ALLOCATION_H_

#include "batch/txn_factory.h"
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

  double time_batched_allocation(unsigned int number_of_txn, unsigned int batch_size) {
    ActionFactory<RMWBatchAction> act_factory(get_action_spec());
    BatchActionAllocator<RMWBatchAction> baa(batch_size);
    std::vector<IBatchAction*> acts;
    acts.resize(number_of_txn);

    auto time_it = [&act_factory, &number_of_txn, &acts, &baa]() {
      IBatchAction* curr_act;
      for (unsigned int i = 0; i < number_of_txn; i++) {
        curr_act = baa.get_action();
        act_factory.initialize_txn_to_random_values(curr_act);
        acts.push_back(curr_act);
      }
    };

    return TimeUtilities::time_function_ms(time_it); 
  };

  void time_txn_allocation() {
    TablePrinter tp;
    tp.set_table_header("Transaction Allocation Time [ms]");
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

    auto get_batch_fun_for = [](unsigned int batch_size) {
      return [batch_size](unsigned int txn_num) {
        return time_batched_allocation(txn_num, batch_size);
      };
    };

    exec_and_add_to_table("One-by-one allocation", time_individual_alloation);
    for (auto& batch_size : {10, 100, 1000, 10000}) {
      exec_and_add_to_table(
          "Allocator batch size: " + std::to_string(batch_size),
          get_batch_fun_for(batch_size));
    } 

    tp.print_table();
  };
};

#endif //TIME_TXN_ALLOCATION_H_
