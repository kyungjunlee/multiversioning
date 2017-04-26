#ifndef DATA_OUT_H_
#define DATA_OUT_H_

#include <iostream>
#include <fstream>

class Out {
private:
  std::string write_dir;
  ExperimentConfig config;

  bool file_exists(std::string file_path) {
    std::ifstream f(file_path.c_str());
    return f.good();
  };
  
  std::ofstream open_file(std::string file_path) {
    std::ofstream ofs;
    try {
      ofs.open (file_path.c_str(), std::ofstream::out | std::ofstream::app);
    } catch (const std::ofstream::failure& e) {
      std::cerr << e.what() << std::endl; 
      exit(-1);
    }

    return ofs;
  }

  void write_header(std::ofstream& ofs) {
    ofs << 
      "num_txns,batch_size,num_sched_threads," <<
      "num_exec_threads,num_records,avg_shared_locks," <<
      "std_dev_shared_locks,avg_excl_locks,std_dev_excl_locks," <<
      "result" << std::endl;
  }

  void write_result(std::ofstream& ofs, double result) {
    ofs <<
      config.num_txns << "," <<
      config.sched_conf.batch_size_act << "," <<
      config.sched_conf.scheduling_threads_count << "," <<
      config.exec_conf.executing_threads_count << "," <<
      config.db_conf.tables_definitions[0].num_records << "," <<
      config.act_conf.reads.average_num_locks << "," <<
      config.act_conf.reads.std_dev_of_num_locks << "," <<
      config.act_conf.writes.average_num_locks << "," <<
      config.act_conf.writes.std_dev_of_num_locks << "," <<
      result << std::endl;
  }

public:
  Out(ExperimentConfig config): 
    write_dir(config.output_dir),
    config(config) 
  {
    if (write_dir.back() == '/') write_dir.pop_back();

    assert(write_dir.empty() == false);
  };

  void write_results(std::vector<double> results) {
    std::string file_path = write_dir + "/data";
    bool file_existed = file_exists(file_path);
    auto file_handle = open_file(file_path);

    if (file_existed) {
      std::cerr << 
        "Appending results to existing file: " << 
        file_path << std::endl;
    } else {
      write_header(file_handle);
    }
    
    // write data
    for (auto& data : results) {
      write_result(file_handle, data);
    }

    file_handle.close();
  };

  void write_exp_description() {
    std::string file_path = write_dir + "/description"; 
    unsigned int file_path_num = 0;
    while (file_exists(file_path)) {
      file_path = write_dir + "/description_" + std::to_string(file_path_num++);
      std::cerr << file_path << std::endl;
    }

    if (file_path_num != 0) {
      std::cerr << "File existed. Writing to: " << file_path << 
        " instead to avoid data overwrite." << std::endl;
    }

    auto file_handle = open_file(file_path);

    auto write_desc_row = 
        [&file_handle](std::string description, unsigned int val) {
      file_handle.width(40);
      file_handle << std::left << "\t" + description << std::scientific << val << "\n";
    };

    file_handle << "GENERAL INFORMATION" << std::endl;
    write_desc_row("Transaction number:", config.num_txns);
    write_desc_row("Experiment repetitions:", config.exp_reps);
    file_handle << std::endl;
    
    auto sched_conf = config.sched_conf;
    file_handle << "SCHEDULING SYSTEM" << std::endl;
    write_desc_row("Scheduling threads:", sched_conf.scheduling_threads_count);
    write_desc_row("Batch size (act):", sched_conf.batch_size_act);
    write_desc_row("Batch length (sec):", sched_conf.batch_length_sec);
    write_desc_row("First pin cpu id:", sched_conf.first_pin_cpu_id);
    file_handle << std::endl; 

    auto exec_conf = config.exec_conf;
    file_handle << "EXECUTING SYSTEM" << std::endl;
    write_desc_row("Executing threads:", exec_conf.executing_threads_count);
    write_desc_row("First pin cpu id:", exec_conf.first_pin_cpu_id);
    file_handle << std::endl;

    auto tables = config.db_conf.tables_definitions;
    file_handle << "DATABASE STORAGE" << std::endl;
    write_desc_row("Tables number:", tables.size());
    write_desc_row("Records in table:", tables[0].num_records);
    file_handle << std::endl;

    auto reads = config.act_conf.reads;
    file_handle << "READ LOCKS REQUESTED INFORMATION" << std::endl;
    write_desc_row("Locks requested from:", reads.low_record);
    write_desc_row("Locks requested to:", reads.high_record);
    write_desc_row("Average # of locks requested:", reads.average_num_locks);
    write_desc_row("Std Dev of # of locks requested:", reads.std_dev_of_num_locks);

    auto writes = config.act_conf.writes;
    file_handle << "WRITE LOCKS REQUESTED INFORMATION" << std::endl;
    write_desc_row("Locks requested from:", writes.low_record);
    write_desc_row("Locks requested to:", writes.high_record);
    write_desc_row("Average # of locks requested:", writes.average_num_locks);
    write_desc_row("Std Dev of # of locks requested:", writes.std_dev_of_num_locks);
  
    file_handle.close();
  };
};

#endif // DATA_OUT_H_
