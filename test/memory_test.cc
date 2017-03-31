#include "gtest/gtest.h"
#include "batch/supervisor.h"
#include "batch/batch_action.h"
#include "test/db_test_helper.h"

// Very small test to see if anything works
TEST(MemoryTest, SmallTestOneSchedOneExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 100)
    .set_exec_thread_num(1)
    .set_sched_thread_num(1)
    .set_batch_size(100)
    .set_action_num(1000);

  hp.runTest();
}

// Larger test, but still one scheduler and one executor
TEST(MemoryTest, LargerTestOneSchedOneExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 200)
    .set_exec_thread_num(1)
    .set_sched_thread_num(1)
    .set_batch_size(200)
    .set_action_num(10000);

  hp.runTest();
}

// Small Test, one scheduler two executors.
TEST(MemoryTest, SmallTestOneSchedTwoExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 100)
    .set_exec_thread_num(2)
    .set_sched_thread_num(1)
    .set_batch_size(200)
    .set_action_num(1000);

  hp.runTest();
} 

// Small Test, two schedulers two executors.
TEST(MemoryTest, SmallTestTwoSchedTwoExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 100)
    .set_exec_thread_num(1)
    .set_sched_thread_num(2)
    .set_batch_size(200)
    .set_action_num(1000);

  hp.runTest();
} 

// Larger Test, two schedulers two executors.
TEST(MemoryTest, LargerTestTwoSchedTwoExec) {
  DBTestHelper<Supervisor> hp;
  hp.set_table_info(1, 200)
    .set_exec_thread_num(2)
    .set_sched_thread_num(2)
    .set_batch_size(200)
    .set_action_num(10000);

  hp.runTest();
} 
