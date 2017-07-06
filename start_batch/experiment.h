#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include "experiment_config.h"
#include "batch/txn_factory.h"
#include "batch/memory_pools.h"
#include "batch/time_util.h"
#include "batch/stat_vec.h"
#include "batch/static_mem_conf.h"

#include <cassert>
#include <chrono>
#include <thread>
#include <vector>

#define ACTION_TYPE RMWBatchAction

// Experiment
//  
//  Class that controls the experiment, performs initialization, measurements and
//  data dumps when necessary.
class Experiment {
private:
  typedef TimeUtilities::TimePoint TimePoint;
  typedef StaticVector<
      StaticVector<IBatchAction*, EXEC_BATCH_SIZE>,
      OUTPUT_BATCH_NUM> 
    OutputVector;

  ExperimentConfig conf;
  std::vector<IBatchAction*> workload;
  ActionMemoryPool<ACTION_TYPE> action_pool;
  ActionFactory<ACTION_TYPE> action_factory;
  Supervisor s;

  OutputVector output;
  uint64_t txns_completed;
  unsigned int expected_output_elts;
  bool print_debug;

  void print_debug_info(std::string text_segment) {
    print_debug_info(std::vector<std::string>{text_segment});
  };

  void print_debug_info(std::vector<std::string> text_segment) {
    if (!print_debug) return;

    for (auto& str : text_segment) {
      std::cout.width(20);
      std::cout << str << std::flush;
    }
  };

  void do_warm_up_run() {
    assert(txns_completed == 0);
    assert(conf.num_txns == workload.size());
    print_debug_info("Beginning warm up run ... ");
  
    // we make sure all of the transactions have gone through the DB at least once.
    unsigned int txns_num = workload.size();
    s.set_simulation_workload(std::move(workload));
    workload.clear();

    // prepare all of the variables necessary later on.
    output.clear();
    TimePoint time_start, time_end;
    time_start = TimeUtilities::now();

    barrier();

    s.start_system();

    barrier();

   while (txns_completed < txns_num) {
      auto o = s.get_output();
      if (o.size() == 0) continue;

      if (txns_completed == 0) {
        print_debug_info(
            {
              "\n\tFirst transaction through after: ",
              std::to_string(
                TimeUtilities::time_difference_ms(
                  time_start, TimeUtilities::now())) + "ms\n"
            });
      }

      txns_completed += o.size();
      output.push_back(std::move(o));
    }

    barrier();
    assert(txns_completed == txns_num);
    time_end = TimeUtilities::now();
    print_debug_info(
      {
        "\tWarm up finished within: ",
        std::to_string(
            TimeUtilities::time_difference_ms(
              time_start, time_end)) + "ms\n"
      });
  
    // reset the system and the workload.
    txns_completed = 0;
    for (auto& output_vector : output) {
      for (auto& act : output_vector) {
        action_pool.free(act);
      }
    };
    allocate_workload();

    s.reset_system();
  };

  std::vector<std::pair<double, unsigned int>> do_measurements() {
    assert(workload.size() == conf.num_txns);
    assert(txns_completed == 0);

    uint64_t end_of_measurement = 0;
    std::vector<std::pair<double, unsigned int>> results;
    results.reserve(300);
    TimePoint all_start, input_stop, output_stop, measure_stop;
    auto measure_throughput = [&]() {
      pin_thread(78);
      unsigned int current_measurement = 0;
      unsigned int former_measurement = 0;
      TimePoint iteration_start, iteration_end;
      while(!end_of_measurement) {
        iteration_start = TimeUtilities::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        barrier();
        current_measurement = txns_completed;
        iteration_end = TimeUtilities::now();
        results.push_back(
            std::make_pair(
              TimeUtilities::time_difference_ms(iteration_start, iteration_end),
              current_measurement - former_measurement));
        former_measurement = current_measurement;
      }
      measure_stop = TimeUtilities::now();
    };
  
    auto put_input = [&]() {
      pin_thread(77);
      s.set_simulation_workload(std::move(workload));
      input_stop = TimeUtilities::now();
      workload.clear();
    };

    auto get_output = [&]() {
      pin_thread(76);
      output.clear();
      unsigned int workload_size = workload.size();
      uint64_t cur_txns_completed = txns_completed;
      assert(cur_txns_completed == 0);
      while (txns_completed < workload_size) {
        auto o = s.get_output();
        if (o.size() == 0) continue;

        cur_txns_completed = txns_completed;
        bool res = cmp_and_swap(&txns_completed, cur_txns_completed, txns_completed + o.size());
        assert(res);
        output.push_back(std::move(o));
      }

      fetch_and_increment(&end_of_measurement);
      output_stop = TimeUtilities::now();

      barrier();
      // clean up the memory
      for (auto& output_vector : output) {
        for (auto& act : output_vector) {
          action_pool.free(act);
        }
      }
    };

    all_start = TimeUtilities::now();
    std::thread measure(measure_throughput);
    std::thread get_output_thr(get_output);
    std::thread input(put_input);
  
    input.join();
    get_output_thr.join();
    measure.join();

    // stop system
    s.stop_system();

    print_debug_info ({
      "Input Time",
      "Output Time",
      "Measure Time\n",
      std::to_string(TimeUtilities::time_difference_ms(all_start, input_stop)) + "ms",
      std::to_string(TimeUtilities::time_difference_ms(all_start, output_stop)) + "ms",
      std::to_string(TimeUtilities::time_difference_ms(all_start, measure_stop)) + "ms\n"
    });

    return results;
  };

  void allocate_workload() {
    print_debug_info("Allocating workload ... ");
    double time_to_allocate = TimeUtilities::time_function_ms(
        [this]() {
          workload.reserve(conf.num_txns);
          for (unsigned int i = 0; i < conf.num_txns; i++) {
            auto cur_txn = action_pool.alloc_and_initialize();
            action_factory.initialize_txn_to_random_values(cur_txn);
            workload.push_back(cur_txn);
          };
        });
  
    print_debug_info(
      "[ O K ] (" + std::to_string(time_to_allocate) + ")\n");
  };

  void initialize() {
    TimePoint time_start, time_end;
    allocate_workload();

    print_debug_info("Initializing supervisor ... ");
    auto time_to_init = TimeUtilities::time_function_ms([this](){s.init_system();});
    print_debug_info(
      "[ O K ] (" + std::to_string(time_to_init) + ")\n");
  
    // the number of output elts is the number of all batches times the
    // number of executing threads since we partition every workload among
    // all the executing threads. Of course, this is an upper bound which
    // assumes batch_size > number of exec threads
    expected_output_elts =
      conf.num_txns / conf.sched_conf.batch_size_act *
        conf.exec_conf.executing_threads_count;
  };

public:
  Experiment(ExperimentConfig conf, bool print):
    conf(conf),
    action_factory(conf.act_conf),
    s(conf.db_conf, conf.sched_conf, conf.exec_conf),
    txns_completed(0),
    print_debug(print)
  {};

  ~Experiment() {}

  void do_experiment() {
    initialize();
    do_warm_up_run();
    auto results = do_measurements();
  
    Out printer(conf);
    printer.write_exp_description();
    printer.write_interim_completion_time_results(results);
  };
};

#endif // _EXPERIMENT_H_
