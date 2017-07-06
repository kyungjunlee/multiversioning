#ifndef STATIC_MEM_CONFIG_H_
#define STATIC_MEM_CONFIG_H_

#define TXN_NUMBER 500000
#define BATCH_SIZE 1000
#define MAX_REQUESTERS_PER_LOCK_STAGE 30
#define MAX_ACTION_RWSET_SIZE 300
#define EXEC_THREAD_NUM 3

// The execution thread batch is just "ceil(batch_size/exec_thread_num)"
// The +1 is just to make sure!
#define EXEC_BATCH_SIZE (BATCH_SIZE / EXEC_THREAD_NUM + 1)

// worst case scenario every batch is divided across all of the execution
// threads
#define OUTPUT_BATCH_NUM (TXN_NUMBER / BATCH_SIZE * EXEC_THREAD_NUM)

#endif // STATIC_MEM_CONFIG_H_
