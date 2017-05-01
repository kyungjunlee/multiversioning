mkdir -p timing_stuff
mkdir -p timing_stuff/data

source experiment_config

make batch

for batch_size_indicator in `seq 0 $((${#batch_sizes[@]} - 1))`;
  do for threads_indicator in `seq 0 $((${#exec_array[@]} - 1))`; 
		do 
      sched_threads=${sched_array[$threads_indicator]}
      exec_threads=${exec_array[$threads_indicator]}
      batch_size=${batch_sizes[$batch_size_indicator]}

			numactl --interleave=all perf record -a -g build/batch_db --num_txns $num_txns --exp_reps $exp_reps --output_dir timing_stuff/data --batch_size $batch_size --num_sched_threads $sched_threads --num_exec_threads $exec_threads --num_records $db_recs --avg_shared_locks $shared_lock_num --std_dev_shared_locks $shared_std_dev --avg_excl_locks $excl_lock_num --std_dev_excl_locks $excl_std_dev; 

			perf script | FlameGraph/stackcollapse-perf.pl > FlameGraph/out.perf-folded
			FlameGraph/flamegraph.pl FlameGraph/out.perf-folded > timing_stuff/time_perf_${sched_array[$i]}.${exec_array[$i]}.$batch_size.svg 
	done; 
done;
