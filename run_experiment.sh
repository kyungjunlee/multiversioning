mkdir -p data

source experiment_config

make batch;

for batch_size_indicator in `seq 0 ${#batch_sizes[@]}`;
	do for threads_indicator in `seq 0 ${#exec_array[@]}`; 
		do 
      sched_threads=${sched_array[$threads_indicator]}
      exec_threads=${exec_array[$threads_indicator]}
      batch_size=${batch_sizes[$batch_size_indicator]}

			build/batch_db --num_txns 100000 --exp_reps $exp_reps --output_dir data --batch_size $batch_size --num_sched_threads $sched_threads --num_exec_threads $exec_threads --num_records 1000 --avg_shared_locks 10 --std_dev_shared_locks 0 --avg_excl_locks 10 --std_dev_excl_locks 0; 
	done; 
done;
