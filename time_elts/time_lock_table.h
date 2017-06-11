#ifndef TIME_LOCK_TABLE_H_
#define TIME_LOCK_TABLE_H_

#include "batch/db_storage_interface.h"
#include "batch/lock_table.h"
#include "batch/scheduler.h"
#include "batch/txn_factory.h"
#include "batch/RMW_batch_action.h"
#include "batch/batch_action_interface.h"
#include "time_utilities.h"

#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>

#define DB_TABLES 1
#define DB_RECORDS 1000

namespace TimeLockTable {
  DBStorageConfig get_db_config() {
    // prepare the db configuration object
    DBStorageConfig db_conf;
    db_conf.tables_definitions.resize(DB_TABLES);
    for (unsigned int i = 0; i < db_conf.tables_definitions.size(); i++) {
      auto& conf = db_conf.tables_definitions[i];
      conf.table_id = i;
      conf.num_records = DB_RECORDS;
    }

    return db_conf;
  };

  double time_merge(unsigned int txns_in_batch, unsigned int batches) {
    // prepare actions.
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

    std::vector<std::unique_ptr<IBatchAction>> actions = 
      ActionFactory<RMWBatchAction>::generate_actions(as, batches * txns_in_batch); 

    std::vector<BatchLockTable> blts;
    blts.reserve(batches);
    // Use scheduler class to create the necessary blts.
    Scheduler st(nullptr, 0, 0);
    for (unsigned int i = 0; i < batches; i++) {
      std::vector<std::unique_ptr<IBatchAction>> batch;
      for (unsigned int j = 0; j < txns_in_batch; j++) {
        batch.push_back(std::move(actions[i*txns_in_batch + j]));
      }

      st.batch_actions.batch = std::move(batch);
      st.batch_actions.batch_id = 0;
      st.process_batch();
      blts.push_back(std::move(st.lt));
    }

    // Get the lock table into which we will be merging.
    LockTable lt(get_db_config());

    // now we have the corrects blts and we may proceed to time the merging process.
    auto time_it = [&blts, &lt]() {
      for (unsigned int i = 0; i < blts.size(); i++) {
        lt.merge_batch_table(blts[i]);
      }
    };

    return TimeUtilities::time_function_ms(time_it); 
  };

  void time_lock_table() {
    TablePrinter tp;
    tp.set_table_header("Lock Table Merging Timing [ms]");
    tp.add_column_headers({
        "Txns per batch",
        "10 batches",
        "100 batches",
        "1 000 batches",
    });

    auto exec_and_add_to_table = [&tp](
        std::string row_name,
        std::function<double (unsigned int)> fun) {
      tp.add_row();
      tp.add_to_last_row(row_name);

      auto time_and_add = [&fun, &tp](unsigned int batches) {
        double result = fun(batches); 
        double per_fun_exec = result/batches;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(6) <<
          result << "(" << per_fun_exec << ")";
        tp.add_to_last_row(ss.str());
      }; 

      for (auto& batches : {10, 100, 1000}) {
        time_and_add(batches);
      }
    };

    auto get_fun_for = [](unsigned int txns){
      return [txns](unsigned int batches) {
        return time_merge(txns, batches);
      };
    }; 

    for (auto& txn_num : {10, 100, 1000, 2000}) {
      exec_and_add_to_table(
          std::to_string(txn_num) + " txns per batch",
          get_fun_for(txn_num));
    }

    tp.print_table();
  }
};

#endif // TIME_LOCK_TABLE_H_
