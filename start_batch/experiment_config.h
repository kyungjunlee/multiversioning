#ifndef _EXPERIMENT_CONFIG_H_
#define _EXPERIMENT_CONFIG_H_

#include "batch/executor_system.h"
#include "batch/scheduler_system.h"
#include "batch/db_storage_interface.h"
#include "batch/txn_factory.h"

struct ExperimentConfig {
  SchedulingSystemConfig sched_conf;
  ExecutingSystemConfig exec_conf;
  DBStorageConfig db_conf;
  ActionSpecification act_conf;
  unsigned int num_txns;
  std::string output_dir;

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

#endif //_EXPERIMENT_CONFIG_H_
