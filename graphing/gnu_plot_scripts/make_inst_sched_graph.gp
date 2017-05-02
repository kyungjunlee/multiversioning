# Expected Params:
#   1. path -- the directory in which the tmp files live.
#   2. out_path -- the directory into which to write.
set datafile separator ","

batch_sizes = system('tail -n +2 '.path.' | cut -f 3 -d "," | sort -V | uniq'); 
set output out_path."/inst_exec_graph"

set term png size 1200, 750
set style data linespoints

set key title "batch sizes"
set ylabel "Throughput [txns/ms]"
set xlabel "Scheduling Threads"
set title "Throughput at instant execution"
set pointsize 3

get_data_for(batch_size) = sprintf("<awk -F, '{if (\$3 == %s) print}' %s", batch_size, path)
plot for [batch_size in batch_sizes] get_data_for(batch_size) using 1:2 title batch_size 
