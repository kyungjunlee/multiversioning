set datafile separator ","
set key autotitle columnhead
set term png size 1200, 250*(num_graphs/2 + num_graphs%2)
set output "mean_throughput_by_batch_size"
set size 9.0, 9.0
set multiplot layout (num_graphs / 2) + (num_graphs %2), 2
do for [i=0:num_graphs-1] {
  file = path."/tmp".i
  stats file every ::0::1 using 4 nooutput
  scheds = STATS_min
  stats file every ::0::1 using 3 nooutput
  execs = STATS_min
  set title sprintf("Sched threads: %d, Exec threads %d", scheds, execs)
  set ylabel "execution time [ms]"
  set xlabel "batch size"

  plot file using 1:2:5 with errorbars 
}
