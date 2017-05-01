#! ENV/bin/python

import pandas
import argparse
import utility

parser = argparse.ArgumentParser()
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);

args = parser.parse_args()

data = utility.read_data_in(args.data_path);
data['through'] = data['txn_completed']/data['time_period']
gr = data.groupby(['num_sched_threads', 'batch_size'])
d = gr.mean().reset_index()
d['std'] = gr.std().reset_index()['through'].astype(int)

utility.create_tmp_dir()
limited_data = utility.get_plot_format(
    d, ['num_sched_threads', 'through', 'std', 'batch_size', 'num_txns'])
utility.write_to_tmp(limited_data)
utility.exec_gnuplot_script(
    "make_inst_sched_graph.gp", 
    ["path", "out_path"], 
    ["\"" + utility.get_tmp_dir_path() + "/tmp0\"",
        "\"" + args.out_path +"\""]) 

utility.destroy_tmp_dir_and_files()
