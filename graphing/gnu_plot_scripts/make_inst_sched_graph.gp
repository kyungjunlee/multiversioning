# Expected Params:
#   1. path -- the directory in which the tmp files live.
#   2. out_path -- the directory into which to write.
set datafile separator ","
set key autotitle columnhead
set term png size 1200, 750
batch_size = system('tail -n +2 '.path.' | cut -f 4 -d "," | uniq'); 
set output out_path."/inst_exec_graph_b".batch_size

set title sprintf("Batch Size: %s", batch_size)
set ylabel "Completion Time [ms]"
set xlabel "Scheduling Threads"
plot path using 1:2:3 with errorbars 

