mkdir -p data
mkdir -p gperf

source experiment_config

make batch -j17

heap_prof=heapprof
cpu_prof=cpuprof

for threads_indicator in `seq 0 $((${#exec_array[@]} - 1))`; 
  do for batch_size_indicator in `seq 0 $((${#batch_sizes[@]} - 1))`;
    do 
      sched_threads=${sched_array[$threads_indicator]}
      exec_threads=${exec_array[$threads_indicator]}
      batch_size=${batch_sizes[$batch_size_indicator]}
      gperf_heapprof=gperf.${sched_threads}.${exec_threads}.${batch_size}
      gperf_cpu=gperf.${sched_threads}.${exec_threads}.${batch_size}.cpu

      if [ "$1" == "$heap_prof" ]; then
        HEAPPROFILE=gperf/${gperf_heapprof}
        export HEAPPROFILE
      elif [ "$1" == "$cpu_prof" ]; then
        CPUPROFILE=gperf/${gperf_cpu}
        export CPUPROFILE
      else
        echo "no gperf parameter given"
      fi

      numactl --interleave=all build/batch_db --num_txns $num_txns --output_dir data --batch_size $batch_size --num_sched_threads $sched_threads --num_exec_threads $exec_threads --num_records $db_recs --avg_shared_locks $shared_lock_num --std_dev_shared_locks $shared_std_dev --avg_excl_locks $excl_lock_num --std_dev_excl_locks $excl_std_dev --num_table_merging_shard $num_table_merging_shard

  done; 
done;
