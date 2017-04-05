#include <getopt.h>

static struct option long_options[] = {
  {"batch_size", required_argument, 0, 0},
  {"num_txns", required_argument, 0, 1},
  {"num_sched_threads", required_argument, 0, 2},
  {"num_exec_threads", required_argument, 0, 3},
  {"num_records", required_argument, 0, 4},
  {"avg_shared_locks", required_argument, 0, 5},
  {"std_dev_shared_locks", required_argument, 0, 6},
  {"avg_excl_locks", required_argument, 0, 7},
  {"std_dev_excl_locks", required_argument, 0, 8},
  {0, no_argument, 0, 9}
};

class ArgParse {
private:
  enum class OptionCode {
    batch_size = 0,
    num_txns,
    num_sched_threads,
    num_exec_threads,
    num_records,
    avg_shared_locks,
    std_dev_shared_locks,
    avg_excl_locks,
    std_dev_excl_locks,
    count
  };

public:

};
