#ifndef TIME_TXN_CREATION_H_
#define TIME_TXN_CREATION_H_

namespace TimeActionCreation {
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

  void time_action_creation() {
    TablePrinter tp;
    tp.set_table_header("Action Initialization Time [ms]");
    tp.add_column_headers({
        "100 reps",
        "10 000 reps",
        "1 000 000 reps"});
    
    tp.add_row();
    auto time_and_add = [&tp](unsigned int reps) {
      RMWBatchAction* action = new RMWBatchAction(new TestTxn());
      ActionFactory<RMWBatchAction> act_factory(get_action_spec());

      auto time_it = [&action, &reps, &act_factory]() {
        for (unsigned int i = 0; i < reps; i++) {
          act_factory.initialize_txn_to_random_values(action);
          action->get_readset_handle()->clear();
          action->get_writeset_handle()->clear();
          // NOTE:
          //    This causes compilation error. just change RMWBATCHACTION if you
          //    really want this. Not a big deal.
          action->tmp_reads.clear();
        }
      };

      double result = 0;
      for (unsigned int i = 0; i < 5; i++) {
        result += TimeUtilities::time_function_ms(time_it);
      }
      result = result / 5;
      double per_fun_exec = result / reps;

      tp.add_to_last_row(
          PrintUtilities::double_to_string(result) + 
            "(" + PrintUtilities::double_to_string(per_fun_exec) + ")"
      );

      delete action;
    };

    for (auto& reps : {100, 10000, 1000000}) {
      time_and_add(reps);
    }

    tp.print_table();
  };
};
#endif // TIME_TXN_CREATION_H_
