#include "gtest/gtest.h"
#include "batch/lock_table.h"
#include "test/test_action.h"
#include "test/test_lock_table.h"

#include <memory>
#include <thread>

TEST(BatchLockTable, constructorTest) {
  BatchLockTable blt;
  ASSERT_EQ(0, blt.get_lock_table_data().size());
}

TEST(BatchLockTable, insert_lock_requestTest) {
  BatchLockTable blt;
  blt.insert_lock_request(
      std::make_shared<TestAction>(*TestAction::make_test_action_with_test_txn({1},{})));
  ASSERT_EQ(1, blt.get_lock_table_data().size());
}

TEST(LockTable, merge_batch_tableTest) {
  BatchLockTable blt;
  blt.insert_lock_request(
      std::make_shared<TestAction>(*TestAction::make_test_action_with_test_txn({1,2,3},{})));
  blt.insert_lock_request(
      std::make_shared<TestAction>(*TestAction::make_test_action_with_test_txn({4,5,6},{})));

  TestLockTable lt;
  lt.merge_batch_table(blt);
  ASSERT_EQ(6, lt.get_lock_table_data().size());
}

TEST(LockTable, concurrent_merge_table_test) {
  unsigned int thread_num = 4;
  unsigned int thread_iterations = 15;

  // used to keep track of the BLTs that are being merged into the 
  // global schedule.
  std::vector<std::shared_ptr<BatchLockTable>> thread_data[thread_num];
  std::thread threads[thread_num];
  // TestLockTable to enable easier inspection of the elt. In the future we might
  // want to change the API so that we create a test object from a LT just so that
  // we have a much cleaner interface. For now we should be good.
  TestLockTable lt;

  auto execute = [thread_iterations, &thread_data, &threads, &lt](int i){
    return [thread_iterations, &thread_data, &threads, i, &lt](){
      std::shared_ptr<BatchLockTable> blt;
      for (unsigned int j = 0; j < thread_iterations; j++) {
        // just two actions on conflicting elements
        blt = std::make_shared<BatchLockTable>();
        blt->insert_lock_request(
            std::make_shared<TestAction>(*TestAction::make_test_action_with_test_txn({1,2,3},{4,5})));
        blt->insert_lock_request(
            std::make_shared<TestAction>(*TestAction::make_test_action_with_test_txn({4,5},{1,2})));

        thread_data[i].push_back(blt);
        lt.merge_batch_table(*blt);
      }
    };
  };

  // set up the threads and let them run.
  for (unsigned int i = 0; i < thread_num; i++) {
    threads[i] = std::thread(execute(i));
  }

  for (unsigned int i = 0; i < thread_num; i++) {
    threads[i].join();
  }

  // make sure that every lock stage added by every thread to every queue is present
  // in the global schedule at the end of the day. 
  LockQueue::QueueElt* currElt;
  for (unsigned int i = 0; i < thread_num; i++) {
    for (unsigned int j = 0; j < thread_iterations; j++) {
      // iterate over all elt within the batch lock tables of a given thread.
      for (const auto& elt : thread_data[i][j]->get_lock_table_data()) {
        currElt = elt.second->peek_head_elt();
        while (currElt != nullptr) {
          ASSERT_TRUE(lt.lock_table_contains_stage(elt.first, currElt->get_contents()));
          currElt = currElt->get_next_elt();
        }
      }
    } 
  }  
  ASSERT_EQ(5, lt.get_lock_table_data().size());
};
