#ifndef _ARG_PARSE_H_
#define _ARG_PARSE_H_

#include "batch/executor_system.h"
#include "batch/scheduler_system.h"
#include "batch/db_storage_interface.h"
#include "batch/txn_factory.h"

#include <getopt.h>
#include <string>
#include <stdlib.h>
#include <unordered_set>
#include <fstream>
#include <iostream>

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
  {"exp_reps", required_argument, 0, 9},
  {"output_dir", required_argument, 0, 10},
  {"exp_type", required_argument, 0, 11},
  {0, no_argument, 0, 12}
};

struct ExperimentConfig {
  SchedulingSystemConfig sched_conf;
  ExecutingSystemConfig exec_conf;
  DBStorageConfig db_conf;
  ActionSpecification act_conf;
  unsigned int num_txns;
  unsigned int exp_reps;
  std::string output_dir;
  std::unordered_set<std::string> exps;

  std::ofstream& print_experiment_header(std::ofstream& ofs) {
    ofs << "num_txns,batch_size,num_sched_threads," <<
      "num_exec_threads,num_records,avg_shared_locks," <<
      "std_dev_shared_locks,avg_excl_locks," <<
      "std_dev_excl_locks";

    return ofs;
  };

  std::ofstream& print_experiment_values(std::ofstream& ofs) {
    ofs <<
      num_txns << "," <<
      sched_conf.batch_size_act << "," <<
      sched_conf.scheduling_threads_count << "," <<
      exec_conf.executing_threads_count << "," <<
      db_conf.tables_definitions[0].num_records << "," <<
      act_conf.reads.average_num_locks << "," <<
      act_conf.reads.std_dev_of_num_locks << "," <<
      act_conf.writes.average_num_locks << "," <<
      act_conf.writes.std_dev_of_num_locks;

    return ofs;
  };

  std::ofstream& print_experiment_info(std::ofstream& ofs) {
    auto write_desc_of_a_row = 
        [&ofs](std::string description) -> std::ofstream& {
      ofs.width(40);
      ofs << std::left << "\t" + description;
      
      return ofs;
    };

    auto write_desc_row = 
        [&write_desc_of_a_row, &ofs](std::string description, unsigned int val) {
      write_desc_of_a_row(description) << std::scientific << val << "\n";
    };

    ofs << "GENERAL INFORMATION" << std::endl;
    write_desc_row("Transaction number:", num_txns);
    write_desc_row("Experiment repetitions:", exp_reps);
    write_desc_of_a_row("Experiments run:");
    std::string experiments_run = "";
    for (auto& exp : exps) {
      if (experiments_run != "") {
        experiments_run += ",";
      }
      
      experiments_run += exp;
    }
    write_desc_of_a_row("Experiments run") << experiments_run << std::endl;
    
    ofs << "SCHEDULING SYSTEM" << std::endl;
    write_desc_row("Scheduling threads:", sched_conf.scheduling_threads_count);
    write_desc_row("Batch size (act):", sched_conf.batch_size_act);
    write_desc_row("Batch length (sec):", sched_conf.batch_length_sec);
    write_desc_row("First pin cpu id:", sched_conf.first_pin_cpu_id);
    ofs << std::endl; 

    ofs << "EXECUTING SYSTEM" << std::endl;
    write_desc_row("Executing threads:", exec_conf.executing_threads_count);
    write_desc_row("First pin cpu id:", exec_conf.first_pin_cpu_id);
    ofs << std::endl;

    auto tables = db_conf.tables_definitions;
    ofs << "DATABASE STORAGE" << std::endl;
    write_desc_row("Tables number:", tables.size());
    write_desc_row("Records in table:", tables[0].num_records);
    ofs << std::endl;

    auto reads = act_conf.reads;
    ofs << "READ LOCKS REQUESTED INFORMATION" << std::endl;
    write_desc_row("Locks requested from:", reads.low_record);
    write_desc_row("Locks requested to:", reads.high_record);
    write_desc_row("Average # of locks requested:", reads.average_num_locks);
    write_desc_row("Std Dev of # of locks requested:", reads.std_dev_of_num_locks);

    auto writes = act_conf.writes;
    ofs << "WRITE LOCKS REQUESTED INFORMATION" << std::endl;
    write_desc_row("Locks requested from:", writes.low_record);
    write_desc_row("Locks requested to:", writes.high_record);
    write_desc_row("Average # of locks requested:", writes.average_num_locks);
    write_desc_row("Std Dev of # of locks requested:", writes.std_dev_of_num_locks);

    return ofs;
  };
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
    exp_reps,
    output_dir,
    exp_type,
    count
  };

  typedef std::unordered_map<int, char*> ArgMap;
  static ArgMap get_arg_map(int argc, char** argv) {
    ArgMap arg_map;

    int c = 0;
    while (getopt_long(argc, argv, "", long_options, &c) != -1) {
      if (c != -1 && arg_map.count(c) == 0) {
        // correct argument
        arg_map[c] = optarg;
      } else if (c == -1) {
        // argument unknown
        std::cerr << "arg_parse.h: Unknown argument.\n";
        exit(-1);
      } else {
        if (long_options[c].name == std::string("exp_type")) {
          arg_map[c] = (char*) (std::string(arg_map[c]) + " " + std::string(optarg)).c_str();
        } else {
          // duplicate argument
          std::cerr << "arg_parse.h: Duplicate argument " << 
            long_options[c].name << ".\n";
          exit(-1);
        }
      }
    }

    return arg_map;
  };

  static void check_value_in_legal_options(
      std::string err_string,
      std::string value,
      std::unordered_set<std::string> legal_options) {
    if (legal_options.count(value) == 0) {
      std::cerr << "Illegal value: " << value <<
        " for: " << err_string << ". Legal values are:" <<
        std::endl;

      for (auto& elt : legal_options) {
        std::cerr << "--" << elt << std::endl;
      }

      exit(-1);
    }
  };

  static void check_presence(
      ArgMap m, 
      std::string system_name,
      std::vector<OptionCode> elts) {
    auto get_error_string = [&m](OptionCode key){
      int k = static_cast<int>(key);
      if (m.count(k) == 0) {
        std::string opt_name(long_options[k].name);
        return "\t--" + opt_name + "\n";
      }

      return std::string("");
    };

    std::string error_string = "";
    for (auto& key : elts) {
      error_string += get_error_string(key);
    }

    // no error
    if (error_string.empty()) return;

    std::cerr 
      << "Missing the following parameters for the " 
      << system_name
      << "." << std::endl
      << error_string
      << std::endl;

    exit(-1);
  };

  static SchedulingSystemConfig get_sched_conf(ArgMap m) {
    check_presence(
        m, "scheduling system", 
        {OptionCode::batch_size, OptionCode::num_sched_threads});

    SchedulingSystemConfig conf = {
      .scheduling_threads_count = 
        (uint32_t) strtoul(
            m[static_cast<int>(OptionCode::num_sched_threads)], nullptr, 10),
      .batch_size_act = 
        (uint32_t) strtoul(
            m[static_cast<int>(OptionCode::batch_size)], nullptr, 10),
      .batch_length_sec = 0,
      .first_pin_cpu_id = 1
    };

    return conf;
  };

  static ExecutingSystemConfig get_exec_conf(ArgMap m) {
    check_presence(
        m, "executing system", 
        {OptionCode::num_exec_threads, OptionCode::num_sched_threads});

    ExecutingSystemConfig conf = {
      .executing_threads_count = 
        (uint32_t) strtoul(
            m[static_cast<int>(OptionCode::num_exec_threads)], nullptr, 10),
      .first_pin_cpu_id = 
        (uint32_t) strtoul(
            m[static_cast<int>(OptionCode::num_sched_threads)], nullptr, 10) + 1
    };

    return conf;
  };

  static DBStorageConfig get_db_conf(ArgMap m) {
    check_presence(
        m, "storage",
        {OptionCode::num_records});

    DBStorageConfig conf = {{{
      .table_id = 0, 
      .num_records = 
        (uint64_t) strtoul(m[static_cast<int>(OptionCode::num_records)], nullptr, 10)
    }}};

    return conf;
  };

  static ActionSpecification get_act_spec(ArgMap m) {
    check_presence(
        m, "action specification",
        {OptionCode::avg_shared_locks,
          OptionCode::std_dev_shared_locks,
          OptionCode::avg_excl_locks,
          OptionCode::std_dev_excl_locks,
          OptionCode::num_txns});

    // TODO: We for now assume that we are using all elts possible.
    ActionSpecification as = {
      .writes = {
        .low_record = 0,
        .high_record = 
          (unsigned int) strtoul(
              m[static_cast<int>(OptionCode::num_records)], nullptr, 10) - 1,
        .average_num_locks = 
          (unsigned int) strtoul(
              m[static_cast<int>(OptionCode::avg_shared_locks)], nullptr, 10),
        .std_dev_of_num_locks =
          (unsigned int) strtoul(
              m[static_cast<int>(OptionCode::std_dev_shared_locks)], nullptr, 10)
      },
      .reads = {
        .low_record = 0,
        .high_record = 
          (unsigned int) strtoul(
              m[static_cast<int>(OptionCode::num_records)], nullptr, 10) - 1,
        .average_num_locks = 
          (unsigned int) strtoul(
              m[static_cast<int>(OptionCode::avg_excl_locks)], nullptr, 10),
        .std_dev_of_num_locks =
          (unsigned int) strtoul(
              m[static_cast<int>(OptionCode::std_dev_excl_locks)], nullptr, 10)
      }
    };

    return as;
  };

  static std::unordered_set<std::string> get_experiment_types(ArgMap m) {
    check_presence(
        m, "experiment types",
        {OptionCode::exp_type});

    std::string exps = m[static_cast<int>(OptionCode::exp_type)];
    std::string tmp;
    std::unordered_set<std::string> res;
    for (unsigned int i = 0; i < exps.length(); i++) {
      if (exps[i] == ' ') {
        if (legal_experiment_types.count(tmp) == 0) {
          
        }
        res.insert(tmp);
        tmp = "";
      }

      tmp += exps[i];
    }

    return res;
  };

public:
  static const std::unordered_set<std::string> legal_experiment_types;

  static ExperimentConfig parse_args(int argc, char** argv) {
    auto arg_map = get_arg_map(argc, argv);

    check_presence(
        arg_map, "experiment", 
        {OptionCode::num_txns,
        OptionCode::exp_reps,
        OptionCode::output_dir,
        OptionCode::exp_type});

    ExperimentConfig exp_conf = {
      .sched_conf = get_sched_conf(arg_map),
      .exec_conf = get_exec_conf(arg_map),
      .db_conf = get_db_conf(arg_map),
      .act_conf = get_act_spec(arg_map),
      .num_txns = 
        (unsigned int) strtoul(
            arg_map[static_cast<int>(OptionCode::num_txns)], nullptr, 10),
      .exp_reps = 
        (unsigned int) strtoul(
            arg_map[static_cast<int>(OptionCode::exp_reps)], nullptr, 10),
      .output_dir = 
        std::string(arg_map[static_cast<int>(OptionCode::output_dir)]),
      .exps = get_experiment_types(arg_map)
    };

    return exp_conf;
  }; 
};

const std::unordered_set<std::string> ArgParse::legal_experiment_types = {
    "global_throughput",
    "interim_throughput"
  };


#endif // ARG_PARSE_H_
