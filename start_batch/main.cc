#include "arg_parse.h"
#include "batch/txn_factory.h"
#include "batch/supervisor.h"
#include "batch/RMW_batch_action.h"

#include <ctime>

int main(int argc, char** argv) {
  ExperimentConfig exp_conf = ArgParse::parse_args(argc, argv);
  std::vector<double> results; 

  for (unsigned int i = 0; i < exp_conf.exp_reps; i++) {
    // declare all of the variables necessary up front.
    auto workload = 
      ActionFactory<RMWBatchAction>::generate_actions(
          exp_conf.act_conf, exp_conf.num_txns);
    time_t time_start, time_end;
    std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> outputs;
    unsigned int output_count = 0;

    // set up the system
    Supervisor s(exp_conf.db_conf, exp_conf.sched_conf, exp_conf.exec_conf);
    s.set_simulation_workload(std::move(workload));
    s.init_system();

    barrier();

    // save time.
    time(&time_start);
    
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
    time(&time_end);

    s.stop_system();

    results.push_back(difftime(time_end, time_start));
  }

  // all of the experiments have finished running. Output the results.

  return 0;
}
