#include "arg_parse.h"
#include "data_out.h"
#include "cpuinfo.h"
#include "batch/txn_factory.h"
#include "batch/supervisor.h"
#include "batch/RMW_batch_action.h"
#include "cpuinfo.h"
#include "experiment_config.h"
#include "experiment.h"

#include <chrono>

int main(int argc, char** argv) {
  ExperimentConfig exp_conf = ArgParse::parse_args(argc, argv);
  Experiment exp(exp_conf, true);
  exp.do_experiment();

  return 0;
}
