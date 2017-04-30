# Expected Params:
#   1. path -- the directory in which the tmp files live.
#   2. out_path -- the directory into which to write.
set datafile separator ","
set key autotitle columnhead
set term png size 1200, 750

batch_sizes = system('tail -n +2 '.path.' | cut -f 4 -d "," | sort | uniq'); 
set output out_path."/inst_exec_graph"

set ylabel "Throughput [txns/ms]"
set xlabel "Scheduling Threads"
set title "Throughput at instant execution"

get_data_for(batch_size) = sprintf("<awk -F, '{if (\$4 == %s) print}' %s", batch_size, path)
plot for [batch_size in batch_sizes] get_data_for(batch_size) using 1:2:3 with errorbars title batch_size
