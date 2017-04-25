#include "arg_parse.h"
#include "data_out.h"
#include "cpuinfo.h"
#include "batch/txn_factory.h"
#include "batch/supervisor.h"
#include "batch/RMW_batch_action.h"
#include "cpuinfo.h"

#include <chrono>

int main(int argc, char** argv) {
  pin_thread(79);
  ExperimentConfig exp_conf = ArgParse::parse_args(argc, argv);
  std::vector<double> completion_time_results; 
  // interim_results is a vector of pairs (double, double) or the format
  // time_since_beginning, actions_finished
  typedef std::pair<double, unsigned int> InterimResultPair;
  typedef std::vector<InterimResultPair> InterimResult;
  typedef std::vector<InterimResult> InterimResults;
  InterimResults interim_results(exp_conf.exp_reps);

  for (unsigned int i = 0; i < exp_conf.exp_reps; i++) {
    std::string batch_msg(
        "Iteration " + std::to_string(i + 1) + 
        "/" + std::to_string(exp_conf.exp_reps) + ": ");
    std::cout << std::flush;
    std::cout << batch_msg << "Setup up the system." << std::endl;

    // declare all of the variables necessary up front.
    auto workload = 
      ActionFactory<RMWBatchAction>::generate_actions(
          exp_conf.act_conf, exp_conf.num_txns);
    std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> outputs;
    unsigned int output_count = 0;
    // used for timing the whole experiemnt
    std::chrono::system_clock::time_point time_start, time_end;
    // used for calculating the interim throughput

    // set up the system
    Supervisor s(exp_conf.db_conf, exp_conf.sched_conf, exp_conf.exec_conf);
    s.set_simulation_workload(std::move(workload));
    s.init_system();

    barrier();
    std::cout << batch_msg << "Running the experiment." << std::endl;
    barrier();
    // save time.
    time_start = std::chrono::system_clock::now();
    
    // run the experiment
    s.start_system();
    
    barrier(); 

    // collect the output
    while (output_count != exp_conf.num_txns) {
      auto o = s.get_output();
      if (o == nullptr) continue;

      output_count += o->size();
      outputs.push_back(std::move(o));

      // TODO:
      //    Make the frequency a variable.
      // save the time elapsed every 20% of the batch done
      if (output_count >= exp_conf.num_txns * 0.2 * (interim_results[i].size() + 1)) {
        time_end = std::chrono::system_clock::now();
        // if this is slow, move it to outside of the test and push a vector of 
        // times and counts instead.
        auto duration_ms = 
          std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
        interim_results[i].push_back(std::make_pair(duration_ms, output_count));
      }
    }

    barrier();
    
    std::cout << batch_msg << "Done." << std::endl;
    // end of experiment
    time_end = std::chrono::system_clock::now();

    s.stop_system();

    // time_since_epoch
    auto exp_duration_ms = 
      std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    completion_time_results.push_back(exp_duration_ms);
  }

  // all of the experiments have finished running. Output the completion_time_results.
  Out printer(exp_conf);
  if (exp_conf.exps.count("global_throughput") > 0) {
    printer.write_completion_time_results(completion_time_results);
  } 

  if (exp_conf.exps.count("interim_throughput") > 0) {
    printer.write_interim_completion_time_results(interim_results);
  }

  printer.write_exp_description();

  return 0;
}
