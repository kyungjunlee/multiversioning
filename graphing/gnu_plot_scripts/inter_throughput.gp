# Expected Params:
#   1. path -- the directory in which the tmp files live.
#   2. out_path -- the directory into which to write.

set datafile separator ","

# Get the meta-data necessary for output names
sched_num = system('tail -n +2 '.path.' | cut -f 4 -d "," | sort | uniq');
exec_num = system('tail -n +2 '.path.' | cut -f 5 -d "," | sort | uniq');
set output out_path."/throughput_in_time_s".sched_num.".e.".exec_num

# Get the meta-data necessary for layouts
batch_sizes = system('tail -n +2 '.path.' | cut -f 3 -d "," | sort -V | uniq');

set term png size 1200, 750
set style data linespoints

set key title "batch sizes"
set ylabel "throughput [txn/ms]"
set xlabel "time since start [ms]"
# Plot

get_for_batch_size(bs) = sprintf("<awk -F, '{if (\$3 == %s) print}' %s", bs, path) 
plot for [batch_size in batch_sizes] get_for_batch_size(batch_size) using 1:2 title batch_size 
