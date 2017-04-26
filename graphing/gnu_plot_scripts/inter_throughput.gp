# Expected Params:
#   1. path -- the directory in which the tmp files live.
#   2. out_path -- the directory into which to write.

set datafile separator ","
set nokey

# Get the meta-data necessary for output names
sched_num = system('tail -n +2 '.path.' | cut -f 5 -d "," | sort | uniq');
exec_num = system('tail -n +2 '.path.' | cut -f 6 -d "," | sort | uniq');
set output out_path."/interim_time_series_s".sched_num.".e.".exec_num

# Get the meta-data necessary for layouts
batch_sizes = system('tail -n +2 '.path.' | cut -f 4 -d "," | sort -V | uniq');
num_graphs = words(batch_sizes)

# Set the style of the graph
set term png size 1200, 250*(num_graphs/2 + num_graphs%2)
set size 9.0, 9.0
set style data linespoints
set multiplot layout (num_graphs / 2) + (num_graphs % 2), 2

# Plot
do for [batch_size in batch_sizes] {
  set title sprintf("Batch size: %s", batch_size)
  set ylabel "throughput [txn/ms]"
  set xlabel "time since start [ms]"

  exp_runs = system("tail -n +2 ".path." | cut -f 3 -d ',' | sort | uniq");
  get_nth_run(i) = sprintf("<awk -F, '{if (\$3 == %s && \$4 == %s) print}' %s", i, batch_size, path)
  plot for [exp_run in exp_runs] get_nth_run(exp_run) using 1:2
}
