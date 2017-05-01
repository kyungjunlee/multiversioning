#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include "experiment_config.h"
#include "batch/txn_factory.h"

#include <cassert>
#include <chrono>
#include <thread>

// Experiment
//    
//
//  Class that controls the experiment, performs initialization, measurements and 
//  data dumps when necessary.
class Experiment {
private:
  typedef std::chrono::system_clock::time_point TimePoint;

  ExperimentConfig conf;
  std::vector<std::unique_ptr<IBatchAction>> workload;
  std::vector<std::unique_ptr<IBatchAction>> warm_up_workload;
  TimePoint time_start, time_end;
  Supervisor s;
  uint64_t txns_completed;
  bool print_debug;

  void do_warm_up_run() {
    assert(txns_completed == 0);
    assert(warm_up_workload.size() > 0);
    if (print_debug) std::cout << "Beginning warm up run ... " << std::flush;
    
    // we make sure all of the transactions have gone through the DB at least once.
    std::chrono::system_clock::time_point first_point;
    unsigned int txns_num = warm_up_workload.size();
    s.set_simulation_workload(std::move(warm_up_workload));
    warm_up_workload.clear();

    time_start = std::chrono::system_clock::now(); 

    barrier();

    s.start_system();

    barrier();

    std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> output;
    while (txns_completed < txns_num) {
      auto o = s.get_output();
      if (o == nullptr) continue;

      if (txns_completed == 0) {
        first_point = std::chrono::system_clock::now();
        std::cout << "\tFirst transaction through after: " << 
          time_period_ms(time_start, first_point) <<
          " ms" << std::endl;
      }

      txns_completed += o->size();
      output.push_back(std::move(o));
    }

    barrier();
    assert(txns_completed == txns_num);
    time_end = std::chrono::system_clock::now();
    std::cout << "\tWarm up finished within: " << 
      time_period_ms(time_start, time_end) <<
      " ms" << std::endl;
    txns_completed = 0;
  };

  std::vector<std::pair<double, unsigned int>> do_measurements() {
    assert(workload.size() == conf.num_txns);

    uint64_t end_of_measurement = 0;
    std::vector<std::pair<double, unsigned int>> results;
    TimePoint all_start, input_stop, output_stop, measure_stop;
    auto measure_throughput = [&]() {
      pin_thread(78);
      unsigned int current_measurement = 0;
      while(!end_of_measurement) {
        time_start = std::chrono::system_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        barrier();
        current_measurement = txns_completed;
        time_end = std::chrono::system_clock::now();
        results.push_back(
            std::make_pair(
              time_period_ms(time_start, time_end),
              current_measurement));
      }
      measure_stop = std::chrono::system_clock::now();
    };
    
    auto put_input = [&]() {
      pin_thread(77);
      s.set_simulation_workload(std::move(workload));
      input_stop = std::chrono::system_clock::now();
    };

    auto get_output = [&]() {
      pin_thread(76);
      std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> output;
      unsigned int workload_size = workload.size();
      uint64_t cur_txns_completed = txns_completed;
      assert(cur_txns_completed == 0);
      while (txns_completed < workload_size) {
        auto o = s.get_output();
        if (o == nullptr) continue;

        cur_txns_completed = txns_completed;
        cmp_and_swap(&txns_completed, cur_txns_completed, txns_completed + o->size());  
        output.push_back(std::move(o));
      }

      fetch_and_increment(&end_of_measurement);
      output_stop = std::chrono::system_clock::now();
    };

    all_start = std::chrono::system_clock::now();
    std::thread measure(measure_throughput);
    std::thread output(get_output);
    std::thread input(put_input);
    
    input.join();
    output.join();
    measure.join();

    // stop system
    s.stop_system();

    if (print_debug) {
      std::cout << "Input Time\t\t\t Output Time \t\t\t Measure Time \n" <<
        time_period_ms(all_start, input_stop) << "\t\t\t" <<
        time_period_ms(all_start, output_stop) << "\t\t\t" <<
        time_period_ms(all_start, measure_stop) << std::endl;
    }

    return results;
  };

  double time_period_ms(TimePoint start, TimePoint end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); 
  };

  void initialize() { 
    if (print_debug) std::cout << "Creating workload ... " << std::flush;
    time_start = std::chrono::system_clock::now();
    workload = ActionFactory<RMWBatchAction>::generate_actions(
      conf.act_conf, conf.num_txns);
    if(print_debug) std::cout << " [ OK ]\n";

    if (print_debug) std::cout << "Initializing supervisor ... " << std::flush;
    s.init_system();
    if (print_debug) std::cout << " [ OK ]\n";
  };

public:
  Experiment(ExperimentConfig conf, bool print): 
    conf(conf), 
    s(conf.db_conf, conf.sched_conf, conf.exec_conf),
    txns_completed(0),
    print_debug(print)
  {};

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
