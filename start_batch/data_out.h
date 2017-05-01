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

  void write_header_completion_time(std::ofstream& ofs) {
    config.print_experiment_header(ofs) << ",";
    ofs << "result" << std::endl;
  }

  void write_header_interim_completion_time(std::ofstream& ofs) {
    config.print_experiment_header(ofs) << ",";
    ofs << "time_since_start,txn_completed" << std::endl;
  }

  void write_result_common(std::ofstream& ofs) {
    ofs <<
      config.num_txns << "," <<
      config.sched_conf.batch_size_act << "," <<
      config.sched_conf.scheduling_threads_count << "," <<
      config.exec_conf.executing_threads_count << "," <<
      config.db_conf.tables_definitions[0].num_records << "," <<
      config.act_conf.reads.average_num_locks << "," <<
      config.act_conf.reads.std_dev_of_num_locks << "," <<
      config.act_conf.writes.average_num_locks << "," <<
      config.act_conf.writes.std_dev_of_num_locks;
  }

  void write_result(std::ofstream& ofs, std::vector<double> result) {
    assert(result.size() > 0);

    config.print_experiment_values(ofs) << ",";
    for (unsigned int i = 0; i < result.size(); i ++) {
      ofs << result[i];
      if (i != result.size() - 1) {
        ofs << ",";
      }
    }
    
    ofs << std::endl;
  }

public:
  Out(ExperimentConfig config): 
    write_dir(config.output_dir),
    config(config) 
  {
    if (write_dir.back() == '/') write_dir.pop_back();

    assert(write_dir.empty() == false);
  };

  void write_completion_time_results(std::vector<double> results) {
    std::string file_path = write_dir + "/completion_time_data";
    bool file_existed = file_exists(file_path);
    auto file_handle = open_file(file_path);

    if (file_existed) {
      std::cerr << 
        "Appending results to existing file: " << 
        file_path << std::endl;
    } else {
      write_header_completion_time(file_handle);
    }
    
    // write data
    for (auto& data : results) {
      write_result(file_handle, {data});
    }

    file_handle.close();
  };

  void write_interim_completion_time_results(
      std::vector<std::pair<double, unsigned int>> res) {
    std::string file_path = write_dir + "/interim_completion_time_data"; 
    bool file_existed = file_exists(file_path);
    auto file_handle = open_file(file_path);

    if (file_existed) {
      std::cerr << "Appending results to existing file: " <<
        file_path << std::endl;
    } else {
      write_header_interim_completion_time(file_handle);
    }

    // write data
    for (auto& p : res) {
      write_result(file_handle, {p.first, (double) p.second});
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

    config.print_experiment_info(file_handle);
    file_handle.close();
  };
};

#endif // DATA_OUT_H_
