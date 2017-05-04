#! ENV/bin/python

import pandas
import argparse
import utility

parser = argparse.ArgumentParser()
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);

args = parser.parse_args()

data = utility.read_data_in(args.data_path);
gr = data.groupby(['num_exec_threads', 'num_sched_threads', 'batch_size'])
d = gr.size().reset_index();
d['batch_size'] = gr.mean().reset_index()['batch_size']
sum_data = gr.sum().reset_index();
d['through'] = sum_data['txn_completed']/sum_data['time_period']

utility.create_tmp_dir()
limited_data = utility.get_plot_format(
    d, ['num_exec_threads', 'through', 'batch_size'])
utility.write_to_tmp(limited_data)
utility.exec_gnuplot_script(
    "make_throughput_graph.gp", 
    ["path", "out_path"], 
    ["\"" + utility.get_tmp_dir_path() + "/tmp0\"",
        "\"" + args.out_path +"\""]) 

utility.destroy_tmp_dir_and_files()
