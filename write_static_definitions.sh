function quit_if_var_unset {
  if [ -z "${!1}" ]; then
    echo "'$1' must be set to a non-null string!";
#    exit;
  fi
}

function write_static_definitions {
  important_vars=(
    batch_size
    shared_lock_num
    shared_std_dev
    excl_lock_num
    shared_lock_num
    exec_threads
    num_txns)

  for i in "${important_vars[@]}"
  do
    quit_if_var_unset $i
  done;

  # Everything is set.

  sigma_multiplier=5
  # Let the max r and w set size be within $sigma_multiplier sigma of their mean
  max_rset_size=$(($shared_lock_num + $sigma_multiplier * $shared_std_dev))
  max_wset_size=$(($excl_lock_num + $sigma_multiplier * $excl_std_dev))

  # and the final variable be the simple max of that
  max_rw_set_size=$(( $max_rset_size > $max_wset_size ? $max_rset_size : $max_wset_size))

  # Write to the config file
  config_path="include/batch/static_mem_conf.h"
  if [ ! -f $config_path ]; then
    echo "File $config_path does not exist. Cannot proceed.";
    exit
  fi
 
  sed -i -e "s/ BATCH_SIZE [0-9]*$/ BATCH_SIZE ${batch_size}/g" $config_path
  sed -i -e "s/ MAX_ACTION_RWSET_SIZE [0-9]*$/ MAX_ACTION_RWSET_SIZE ${max_rw_set_size}/g" $config_path
  sed -i -e "s/ EXEC_THREAD_NUM [0-9]*$/ EXEC_THREAD_NUM ${exec_threads}/g" $config_path
  sed -i -e "s/ TXN_NUMBER [0-9]*$/ TXN_NUMBER ${num_txns}/g" $config_path
}
