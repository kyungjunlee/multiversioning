#include "gtest/gtest.h"
#include "batch/supervisor.h"
#include "batch/batch_action.h"
#include "test/test_txn.h"

void runTestFor(
    unsigned int rec_in_table,
    unsigned int exec_thread_num,
    unsigned int sched_thread_num,
    unsigned int batch_size,
    unsigned int batch_timeout,
    unsigned int action_num) {
  
  DBStorageConfig db_conf = {{{0, rec_in_table}}};

  SchedulingSystemConfig sched_conf = {
    sched_thread_num, batch_size, batch_timeout, 1};

  ExecutingSystemConfig exec_conf = {
    exec_thread_num, sched_thread_num + 1};

  Supervisor s(db_conf, sched_conf, exec_conf);
  
  // prepare the workload.
  std::vector<std::unique_ptr<IBatchAction>> workload;
  for (unsigned int i = 0; i < action_num; i ++) {
    workload.push_back(std::move(std::make_unique<BatchAction>(new TestTxn())));
    for (unsigned int j = 0; j < 5; j++) {
      workload[i]->add_write_key((i + j) % rec_in_table);
      workload[i]->add_read_key((i + j + 10) % rec_in_table);
    }
  }

  s.set_simulation_workload(std::move(workload));  

  barrier();

  s.init_system();
  s.start_system();

  barrier();

  std::vector<std::unique_ptr<std::vector<std::shared_ptr<IBatchAction>>>> outputs;
  unsigned int output_count = 0;
  while (output_count != action_num) {
    auto o = s.get_output();
    if (o == nullptr) continue;

    output_count += o->size();
    outputs.push_back(std::move(o));
  }

  s.stop_system();
}

// Very small test to see if anything works:
//    1) Database: 1 table, 100 records.
//    2) Scheduler: 
//        single thread
//        batch size: 100
//    3) Executor: single thread
// Workload: 1000 actions with 10 locks each.
TEST(SystemTest, SmallTestOneSchedOneExec) {
  runTestFor(
      100,
      1,
      1,
      100,
      0,
      1000);
}

// Larger test, but still one scheduler and one executor:
//    1) Database: 1 table, 200 records.
//    2) Scheduler: 
//        single thread
//        batch size: 200
//    3) Executor: single thread
// Workload: 10000 actions with 10 locks each.
TEST(SystemTest, LargerTestOneSchedOneExec) {
  runTestFor(
      200,
      1,
      1,
      100,
      0,
      10000);
}

// Small Test, one scheduler two executors.
///   1) Database: 1 table, 100 records.
//    2) Scheduler: 
//        single thread
//        batch size: 200
//    3) Executor: single thread
// Workload: 1000 actions with 10 locks each.
TEST(SystemTest, SmallTestOneSchedTwoExec) {
  runTestFor(
      200,
      2,
      1,
      100,
      0,
      1000);
} 

// Small Test, two schedulers two executors.
///   1) Database: 1 table, 100 records.
//    2) Scheduler: 
//        single thread
//        batch size: 200
//    3) Executor: single thread
// Workload: 1000 actions with 10 locks each.
TEST(SystemTest, SmallTestTwoSchedTwoExec) {
  runTestFor(
      200,
      2,
      2,
      100,
      0,
      1000);
} 

// Larger Test, two schedulers two executors.
///   1) Database: 1 table, 100 records.
//    2) Scheduler: 
//        single thread
//        batch size: 200
//    3) Executor: single thread
// Workload: 100000 actions with 10 locks each.
TEST(SystemTest, LargerTestTwoSchedTwoExec) {
  runTestFor(
      200,
      2,
      2,
      100,
      0,
      10000);
} 
