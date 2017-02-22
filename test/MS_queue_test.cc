#include "gtest/gtest.h"

#include "test/test_MS_queue.h"

#include <vector>
#include <thread>

// NOTE:
//    All of the tests of MSQueues are done using integers for simplicity.

class MSQueueTest : public testing::Test {
protected:
  std::shared_ptr<TestMSQueue<int>> make_test_queue(int n, int start_elt = 0) {
    std::shared_ptr<TestMSQueue<int>> tmq = std::make_shared<TestMSQueue<int>>();
    for (int i = start_elt; i < start_elt + n; i++) tmq->non_concurrent_push_tail(new int(i));

    return tmq;
  }

  std::shared_ptr<MSQueue<int>> make_queue(int n, int start_elt = 0) {
    auto tmq = make_test_queue(n, start_elt);

    std::shared_ptr<MSQueue<int>> lq = std::make_shared<MSQueue<int>>();
    lq->merge_queue(tmq.get());
    return lq;
  }
};

TEST_F(MSQueueTest, constructorTest) {
  MSQueue<int> lq;
  ASSERT_TRUE(lq.is_empty());
} 

TEST_F(MSQueueTest, TestFixture_non_concurrent_push_tailTest) {
  TestMSQueue<int> tmq;
  std::vector<int*> elts;
  for (int i = 0; i < 100; i ++) {
    elts.push_back(new int(i));
    tmq.non_concurrent_push_tail(elts[i]);

    // the tail moves while the head remains.
    ASSERT_EQ(elts[i], tmq.peek_tail());
    ASSERT_EQ(elts[0], tmq.peek_head());
  }

  // The queue is well formed.
  auto curr = tmq.peek_head_elt();
  for (int i = 0; i < 100; i++) {
    if (i != 99) ASSERT_EQ(curr->get_next_elt()->get_contents(), elts[i+1]);

    curr = curr->get_next_elt();
  }

  ASSERT_EQ(nullptr, tmq.peek_tail_elt()->get_next_elt());
}

TEST_F(MSQueueTest, merge_queueNonconcurrentTest) {
  auto tmq = make_test_queue(100);
  std::shared_ptr<MSQueue<int>> msQueue = std::make_shared<MSQueue<int>>();

  auto checkQueue = [](auto head_elt, int elts){
    auto* curr = head_elt;
    for (unsigned int i = 0; i < elts; i++) {
      if (i != elts-1) ASSERT_EQ(i, *(curr->get_contents()));

      curr = curr->get_next_elt();
    }
  };

  msQueue->merge_queue(tmq.get());

  // should contain all of the elts there 
  ASSERT_EQ(*msQueue->peek_head(), 0);
  ASSERT_EQ(*msQueue->peek_tail(), 99);
  checkQueue(msQueue->peek_head_elt(), 100);

  // repeated merging should leave the former elements unchanged and add more elements.
  tmq = make_test_queue(100, 100);
  msQueue->merge_queue(tmq.get());
  
  ASSERT_EQ(*msQueue->peek_head(), 0);
  ASSERT_EQ(*msQueue->peek_tail(), 199);
  checkQueue(msQueue->peek_head_elt(), 200);
}

TEST_F(MSQueueTest, try_pop_headNonconcurrentTest) {
  auto msQueue = make_queue(2);

  auto head = msQueue->peek_head_elt();
  auto tail = msQueue->peek_tail_elt();
  auto next_head = head->get_next_elt();

  ASSERT_EQ(0, *msQueue->try_pop_head());
  ASSERT_EQ(next_head, msQueue->peek_head_elt());
  ASSERT_EQ(tail, msQueue->peek_tail_elt());

  ASSERT_EQ(1, *msQueue->try_pop_head());
  ASSERT_EQ(nullptr, msQueue->peek_head_elt());
  ASSERT_EQ(nullptr, msQueue->peek_head());
  ASSERT_EQ(nullptr, msQueue->peek_tail_elt());
  ASSERT_EQ(nullptr, msQueue->peek_tail());
}

TEST_F(MSQueueTest, concurrentPopMergeTest) {
  unsigned int elts_count = 100;
  std::thread threads[2];

  for (unsigned int k = 0; k < 100; k++) {
    std::vector<std::shared_ptr<int>> elts(elts_count);
    MSQueue<int> msQueue;

    // create the producer
    threads[0] = std::thread([this, &msQueue, &elts, elts_count](){
      for (unsigned int i = 0; i < elts_count; i++) {
        TestMSQueue<int> tmq; 
        elts[i] = std::make_shared<int>(i);
        tmq.non_concurrent_push_tail(elts[i].get()); 

        msQueue.merge_queue(&tmq);
     }
    }); 

    threads[1] = std::thread([this, &msQueue, &elts, elts_count](){
      int* currDeqInt;
      for (unsigned int i = 0; i < elts_count; i++) {
        // make sure we dequeu
        while((currDeqInt = msQueue.try_pop_head()) == nullptr) ;

        // consistent values from dequeueing
        ASSERT_EQ(elts[i].get(), currDeqInt);
      }
    });

    threads[0].join();
    threads[1].join();
 
    // the queue should be empty by now
    ASSERT_EQ(nullptr, msQueue.peek_head_elt());
    ASSERT_EQ(nullptr, msQueue.peek_head());
    ASSERT_EQ(nullptr, msQueue.peek_tail_elt());
    ASSERT_EQ(nullptr, msQueue.peek_tail());
  }
}
