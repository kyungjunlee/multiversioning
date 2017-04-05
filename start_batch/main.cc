#include "arg_parse.h"
#include "data_out.h"
#include "batch/txn_factory.h"
#include "batch/supervisor.h"
#include "batch/RMW_batch_action.h"

#include <chrono>

int main(int argc, char** argv) {
  ExperimentConfig exp_conf = ArgParse::parse_args(argc, argv);
  std::vector<double> results; 

  for (unsigned int i = 0; i < exp_conf.exp_reps; i++) {
    // declare all of the variables necessary up front.
    auto workload = 
      ActionFactory<RMWBatchAction>::generate_actions(
          exp_conf.act_conf, exp_conf.num_txns);
    std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> outputs;
    unsigned int output_count = 0;
    std::chrono::system_clock::time_point time_start, time_end;

    // set up the system
    Supervisor s(exp_conf.db_conf, exp_conf.sched_conf, exp_conf.exec_conf);
    s.set_simulation_workload(std::move(workload));
    s.init_system();

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
    }

    barrier();
    
    // end of experiment
    time_end = std::chrono::system_clock::now();

    s.stop_system();

    // time_since_epoch
    auto exp_duration_ms = 
      std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
    results.push_back(exp_duration_ms);
  }

  // all of the experiments have finished running. Output the results.
  Out printer(exp_conf);
  printer.write_results(results);
  printer.write_exp_description();

  return 0;
}
