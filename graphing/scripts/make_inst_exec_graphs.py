#! ENV/bin/python

import pandas
import argparse
import utility

parser = argparse.ArgumentParser()
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);

args = parser.parse_args()

data = utility.read_data_in(args.data_path);
gr = data.groupby(['num_sched_threads', 'batch_size'])
d = gr.mean().reset_index()
d['std'] = gr.std().reset_index()['result'].astype(int)

split_data = utility.split_by_unique_vals(d, ['batch_size'])

utility.create_tmp_dir()
which_one = 0;
for df in split_data:
    limited_data = utility.get_plot_format(
            df, ['num_exec_threads', 'result', 'std', 'batch_size'])
    utility.write_to_tmp(limited_data)
    utility.exec_gnuplot_script(
        "make_inst_sched_graph.gp", 
        ["path", "out_path"], 
        ["\"" + utility.get_tmp_dir_path() + "/tmp" + str(which_one) + "\"",
            "\"" + args.out_path +"\""]) 
    which_one += 1

utility.destroy_tmp_dir_and_files()
