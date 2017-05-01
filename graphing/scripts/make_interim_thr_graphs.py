#! ENV/bin/python

import pandas
import argparse
import utility

parser = argparse.ArgumentParser()
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);

args = parser.parse_args()

data = utility.read_data_in(args.data_path)
gr = data.groupby(['num_sched_threads', 'num_exec_threads', 'batch_size'])
data['time'] = gr['time_period'].transform(pandas.Series.cumsum);

data['throughput'] = data['txn_completed'] / data['time_period']
split_data = utility.split_by_unique_vals(
        data, ['num_sched_threads', 'num_exec_threads'])

utility.create_tmp_dir()
for df in split_data:
    limited_data = utility.get_plot_format(
            df, 
            [
                'time', 'throughput',
                'batch_size', 'num_sched_threads', 'num_exec_threads'
            ])
    utility.write_to_tmp(limited_data)

file_index = 0
for df in split_data:
    utility.exec_gnuplot_script(
        "inter_throughput.gp",
        ["path", "out_path"],
        ["\"" + utility.get_tmp_dir_path() + "/tmp" + str(file_index) + "\"",
        "\"" + args.out_path + "\""]
    );
    file_index += 1

utility.destroy_tmp_dir_and_files()
