#! ENV/bin/python

import pandas
import argparse
import utility

parser = argparse.ArgumentParser()
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);

args = parser.parse_args()

data = utility.read_data_in(args.data_path);
d = data.groupby(['num_sched_threads', 'num_exec_threads', 'batch_size']).mean().reset_index();
d['std'] = data.groupby(['num_sched_threads', 'num_exec_threads', 'batch_size']).std().reset_index()['result'].astype(int);
data = d
split_data = utility.split_by_unique_vals(data, ['num_sched_threads', 'num_exec_threads'])

utility.create_tmp_dir()
for df in split_data:
    limited_data = utility.get_plot_format(
            df, ['batch_size', 'result', 'num_exec_threads', 'num_sched_threads', 'std'])
    utility.write_to_tmp(limited_data)

utility.exec_gnuplot_script(
        "mean_throughput_batch_size.gp", 
        ["path", "num_graphs"], 
        ["\"" + utility.get_tmp_dir_path() + "\"", len(split_data)]) 

utility.destroy_tmp_dir_and_files()
