#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include "experiment_config.h"
#include "batch/txn_factory.h"
#include "batch/time_util.h"

#include <cassert>
#include <chrono>
#include <thread>

// Experiment
//    
//  Class that controls the experiment, performs initialization, measurements and 
//  data dumps when necessary.
class Experiment {
private:
  typedef TimeUtilities::TimePoint TimePoint;

  ExperimentConfig conf;
  std::vector<std::unique_ptr<IBatchAction>> workload;
  std::vector<std::unique_ptr<IBatchAction>> warm_up_workload;
  Supervisor s;
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
    assert(warm_up_workload.size() > 0);
    print_debug_info("Beginning warm up run ... ");
    
    // we make sure all of the transactions have gone through the DB at least once.
    unsigned int txns_num = warm_up_workload.size();
    s.set_simulation_workload(std::move(warm_up_workload));
    warm_up_workload.clear();

    TimePoint time_start, time_end;
    time_start = TimeUtilities::now(); 

    barrier();

    s.start_system();

    barrier();

    std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> output;
    output.reserve(expected_output_elts);
    while (txns_completed < txns_num) {
      auto o = s.get_output();
      if (o == nullptr) continue;

      if (txns_completed == 0) {
        print_debug_info(
            {
              "\n\tFirst transaction through after: ", 
              std::to_string(
                TimeUtilities::time_difference_ms(
                  time_start, TimeUtilities::now())) + "ms\n"
            });
      }

      txns_completed += o->size();
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
    txns_completed = 0;
    s.reset_system();
  };

  std::vector<std::pair<double, unsigned int>> do_measurements(std::vector<double> &total_time) {
    assert(workload.size() == conf.num_txns);

    uint64_t end_of_measurement = 0;
    std::vector<std::pair<double, unsigned int>> results;
    results.reserve(300);
    TimePoint all_start, input_stop, output_stop, measure_stop;
    /*
     * pinning the fixed number here might be dangerous,
     * but change this to automtically set this cpu number
     */
    uint32_t total_num_threads = conf.sched_conf.scheduling_threads_count
      + conf.exec_conf.executing_threads_count + 4;
      // TODO: set 4 for now,4 = 1 + 3 (# of scheduler helpers)

    auto measure_throughput = [&]() {
      pin_thread(total_num_threads);
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
      pin_thread(total_num_threads + 2);
      s.set_simulation_workload(std::move(workload));
      input_stop = TimeUtilities::now();
    };

    auto get_output = [&]() {
      pin_thread(total_num_threads + 1);
      std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> output;
      output.reserve(expected_output_elts);
      unsigned int workload_size = workload.size();
      uint64_t cur_txns_completed = txns_completed;
      assert(cur_txns_completed == 0);
      while (txns_completed < workload_size) {
        auto o = s.get_output();
        if (o == nullptr) continue;

        cur_txns_completed = txns_completed;
        bool res = cmp_and_swap(&txns_completed, cur_txns_completed, txns_completed + o->size());  
        assert(res);
        output.push_back(std::move(o));
      }

      fetch_and_increment(&end_of_measurement);
      output_stop = TimeUtilities::now();
    };

    all_start = TimeUtilities::now();
    std::thread measure(measure_throughput);
    std::thread output(get_output);
    std::thread input(put_input);
    
    input.join();
    output.join();
    measure.join();

    // stop system
    s.stop_system();

    /*
     * measure the total execution time
     */
    total_time.push_back(
      TimeUtilities::time_difference_ms(all_start, input_stop));
    total_time.push_back(
      TimeUtilities::time_difference_ms(all_start, output_stop));
    total_time.push_back(
      TimeUtilities::time_difference_ms(all_start, measure_stop));

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

  std::vector<std::unique_ptr<IBatchAction>> allocate_actions() {
    unsigned int acts_per_thread = 500000;
    unsigned int actions_num = conf.num_txns;
    // this is poor-mans ceil.
    unsigned int thread_num = actions_num / acts_per_thread + (actions_num % acts_per_thread != 0);
    std::vector<std::vector<std::unique_ptr<IBatchAction>>> thread_actions{thread_num};
    std::thread threads[thread_num];

    auto thread_alloc = [&actions_num, &thread_actions, &acts_per_thread, this](unsigned int i) {
      // uneven division.
      unsigned int to_produce = acts_per_thread;
      if (actions_num - i * acts_per_thread < acts_per_thread) {
        to_produce = actions_num - i * acts_per_thread;
      }

      thread_actions[i] = std::move(ActionFactory<RMWBatchAction>::generate_actions(
           this->conf.act_conf, to_produce));
    };

    for (unsigned int i = 0; i < thread_num; i++) {
      threads[i] = std::thread(thread_alloc, i);
    }

    for (unsigned int i = 0; i < thread_num; i++) {
      threads[i].join();
    }

    // merge all of the above into a single vector
    std::vector<std::unique_ptr<IBatchAction>> res;
    for (auto& vec : thread_actions) {
      for (auto& act : vec) {
        res.push_back(std::move(act));
      }
    }

    assert(res.size() == actions_num);
    return res;
  };

  void initialize() { 
    TimePoint time_start, time_end;
    auto print_OK_time = [this, &time_start, &time_end]() {
     print_debug_info(
        "[ O K ] (" + 
        std::to_string(
          TimeUtilities::time_difference_ms(time_start, time_end)) + 
        ")\n");
    };

    print_debug_info("Creating workload ... ");
    time_start = std::chrono::system_clock::now();
    workload = allocate_actions();
    time_end = std::chrono::system_clock::now();
    print_OK_time();

    print_debug_info("Creating warm up workload ... ");
    // TODO: Make the size of this a parameter...
    time_start = std::chrono::system_clock::now();
    warm_up_workload = allocate_actions();
    time_end = std::chrono::system_clock::now();
    print_OK_time();

    print_debug_info("Initializing supervisor ... ");
    time_start = std::chrono::system_clock::now();
    s.init_system();
    time_end = std::chrono::system_clock::now();
    print_OK_time();
    
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
    s(conf.db_conf, conf.sched_conf, conf.exec_conf),
    txns_completed(0),
    print_debug(print)
  {};

  void do_experiment() {
    /*
     * to measure the total time; input_time, output_time, measure_time
     */
    std::vector<double> total_time;
    total_time.reserve(10);

    initialize();
    do_warm_up_run();
    auto results = do_measurements(total_time);
    
    Out printer(conf);
    printer.write_exp_description();
    printer.write_interim_completion_time_results(results);
    printer.write_completion_time_results(total_time);
  };
};

#endif // _EXPERIMENT_H_ 
