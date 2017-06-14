#ifndef TIME_LOCK_TABLE_H_
#define TIME_LOCK_TABLE_H_

#include "batch/db_storage_interface.h"
#include "batch/lock_table.h"
#include "batch/scheduler.h"
#include "batch/txn_factory.h"
#include "batch/RMW_batch_action.h"
#include "batch/batch_action_interface.h"
#include "batch/SPSC_MR_queue.h"
#include "batch/time_utilities.h"

#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <thread>

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
    
  std::vector<std::unique_ptr<IBatchAction>> prepare_actions(
      unsigned int txns_num) {

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

    return ActionFactory<RMWBatchAction>::generate_actions(as, txns_num); 
  };

  std::vector<BatchLockTable> prepare_blts(
      std::vector<std::unique_ptr<IBatchAction>>&& actions,
      unsigned int txns_in_batch,
      unsigned int batches) {
    assert(actions.size() == txns_in_batch * batches);

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
    
    return blts; 
  };

  double time_merge(
      const unsigned int txns_in_batch, 
      const unsigned int batches) {
    auto actions = prepare_actions(txns_in_batch * batches);
    auto blts = prepare_blts(std::move(actions), txns_in_batch, batches); 

    // Get the lock table into which we will be merging.
    LockTable lt(get_db_config());

    auto time_it = [&blts, &lt]() {
      for (unsigned int i = 0; i < blts.size(); i++) {
        lt.merge_batch_table(blts[i]);
      }
    };

    return TimeUtilities::time_function_ms(time_it); 
  };

  double time_merge_concurrent(
      const unsigned int txns_in_batch, 
      const unsigned int batches,
      const unsigned int merging_threads = 2) {
    auto actions = prepare_actions(txns_in_batch * batches);
    auto blts = prepare_blts(std::move(actions), txns_in_batch, batches);

    LockTable lt(get_db_config());
    std::thread threads[merging_threads];
    SPSCMRQueue<BatchLockTable> queues[merging_threads];
    // fill up the initial input queue.
    for (unsigned int i = 0; i < blts.size(); i++) {
      queues[0].push_tail(std::move(blts[i]));
    }

    // prepare the correct function.
    const unsigned int recs_per_thread = DB_RECORDS / merging_threads;
    auto merge = 
      [&queues, &batches, &lt, &merging_threads, &recs_per_thread]
      (unsigned int i) 
        {
          // Every thread takes care of about DB_RECORDS / merging_threads 
          // records. 
          RecordKey lo(recs_per_thread * i);
          RecordKey hi(recs_per_thread * (i+1) - 1);
          if (i == merging_threads - 1) {
            hi.key = DB_RECORDS - 1;
          }

          for (unsigned int j = 0; j < batches; j++) {
            // wait for input
            while (queues[i].is_empty());

            auto& curr_blt = *queues[i].peek_head();
            lt.merge_batch_table_for(curr_blt, lo, hi);
            if (i != merging_threads - 1) {
              queues[i+1].push_tail(std::move(curr_blt));
            }

            queues[i].pop_head();
          }
        };

      auto time_it = [&]() {
        for (unsigned int i = 0; i < merging_threads; i++) {
          threads[i] = std::thread(merge, i);
        }

        for (unsigned int i = 0; i < merging_threads; i++) {
          threads[i].join();
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
        double result = 0;

        // repeat and average...
        for (unsigned int i = 0; i < 5; i++) {
          result += fun(batches);
        }

        result = result / 5;
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

    auto get_merge_fun_for = [](unsigned int txns) {
      return [txns](unsigned int batches) {
        return time_merge(txns, batches);
      };
    }; 

    for (auto& txn_num : {10, 100, 1000, 2000}) {
      exec_and_add_to_table(
          std::to_string(txn_num) + " txns per batch",
          get_merge_fun_for(txn_num));
    }

    auto get_conc_merge_fun_for = []
      (unsigned int thread_num, unsigned int txns) {
        return [txns, thread_num](unsigned int batches) {
          return time_merge_concurrent(txns, batches, thread_num);
        };
      };

    for (auto& thread_num : {2, 3, 4}) {
      for (auto& txn_num : {10, 100, 1000, 2000}) {
        exec_and_add_to_table(
          std::to_string(thread_num) +
          " threads, " + std::to_string(txn_num) + " txns per batch",
          get_conc_merge_fun_for(thread_num, txn_num));
      }
    }

    tp.print_table();
  }
};

#endif // TIME_LOCK_TABLE_H_
