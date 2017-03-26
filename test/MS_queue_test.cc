#include "gtest/gtest.h"
#include "batch/MS_queue.h"

#include <vector>
#include <thread>

// NOTE:
//    All of the tests of MSQueues are done using integers for simplicity.

class MSQueueTest : public testing::Test {
protected:
  std::shared_ptr<MSQueue<int>> make_queue(int n, int start_elt = 0) {
    std::shared_ptr<MSQueue<int>> lq = std::make_shared<MSQueue<int>>();
    for (int i = start_elt; i < start_elt + n; i++) {
      lq->push_tail(i);
    }
    
    return lq;
  }
};

TEST_F(MSQueueTest, constructorTest) {
  MSQueue<int> lq;
  ASSERT_TRUE(lq.is_empty());
} 

TEST_F(MSQueueTest, push_tailNonConcurrentTest) {
  MSQueue<int> tmq;
  std::vector<int> elts;
  for (int i = 0; i < 100; i ++) {
    elts.push_back(i);
    tmq.push_tail(elts[i]);

    // the tail moves while the head remains.
    ASSERT_EQ(elts[i], tmq.peek_tail());
    ASSERT_EQ(elts[0], tmq.peek_head());
  }

  // The queue is well formed.
  auto curr = tmq.peek_head_elt();
  for (int i = 0; i < 100; i++) {
    if (i != 99) ASSERT_EQ(elts[i+1], curr->get_next_elt()->get_contents());

    curr = curr->get_next_elt();
  }

  ASSERT_EQ(nullptr, tmq.peek_tail_elt()->get_next_elt());
}

TEST_F(MSQueueTest, merge_queueNonconcurrentTest) {
  auto tmq = make_queue(100);
  std::shared_ptr<MSQueue<int>> msQueue = std::make_shared<MSQueue<int>>();

  auto checkQueue = [](auto head_elt, int elts){
    auto* curr = head_elt;
    for (unsigned int i = 0; i < elts; i++) {
      if (i != elts-1) ASSERT_EQ(i, curr->get_contents());

      curr = curr->get_next_elt();
    }
  };

  msQueue->merge_queue(tmq.get());

  // should contain all of the elts there 
  ASSERT_EQ(0, msQueue->peek_head());
  ASSERT_EQ(99, msQueue->peek_tail());
  checkQueue(msQueue->peek_head_elt(), 100);

  // repeated merging should leave the former elements unchanged and add more elements.
  tmq = make_queue(100, 100);
  msQueue->merge_queue(tmq.get());
  
  ASSERT_EQ(0, msQueue->peek_head());
  ASSERT_EQ(199, msQueue->peek_tail());
  checkQueue(msQueue->peek_head_elt(), 200);
}

TEST_F(MSQueueTest, pop_headNonconcurrentTest) {
  auto msQueue = make_queue(2);

  auto head = msQueue->peek_head_elt();
  auto tail = msQueue->peek_tail_elt();
  auto next_head = head->get_next_elt();

  ASSERT_EQ(0, msQueue->peek_head());
  msQueue->pop_head();
  ASSERT_EQ(next_head, msQueue->peek_head_elt());
  ASSERT_EQ(tail, msQueue->peek_tail_elt());

  ASSERT_EQ(1, msQueue->peek_head());
  msQueue->pop_head();
  ASSERT_EQ(nullptr, msQueue->peek_head_elt());
  ASSERT_EQ(nullptr, msQueue->peek_tail_elt());
}

typedef 
    std::function<void (MSQueue<int>&, std::vector<int>&, int)> 
    pushFunType;
void runConcurrentTest(pushFunType pushFun, unsigned int line_num) {
  unsigned int elts_count = 100; 
  std::thread threads[2];
  std::vector<int> elts(elts_count);
  MSQueue<int> msQueue;

  // create the producer
  threads[0] = std::thread([&msQueue, &elts, elts_count, &pushFun](){
    for (unsigned int i = 0; i < elts_count; i++) {
      pushFun(msQueue, elts, i);
    }
  }); 
  // create the consumer
  threads[1] = std::thread([&msQueue, &elts, elts_count, &line_num](){
    int currDeqInt;
    for (unsigned int i = 0; i < elts_count; i++) {
      // make sure we dequeu
      while (msQueue.is_empty());
      currDeqInt = msQueue.peek_head();

      // consistent values from dequeueing
      ASSERT_EQ(elts[i], currDeqInt) << 
        "From test beginning on line" << line_num;
      msQueue.pop_head();
    }
  });

  threads[0].join();
  threads[1].join();

  // the queue should be empty by now
  ASSERT_TRUE(msQueue.is_empty());
  ASSERT_EQ(nullptr, msQueue.peek_head_elt()) <<
    "From test beginning on line" << line_num;
  ASSERT_EQ(nullptr, msQueue.peek_tail_elt()) <<
    "From test beginning on line" << line_num;
};

TEST_F(MSQueueTest, concurrentPopMergeTest) {
  for (unsigned int k = 0; k < 100; k++) {
    auto mergeFun = [](MSQueue<int>& q, std::vector<int>& e, int i){
      e[i] = i;
      MSQueue<int> tmq;
      tmq.push_tail(e[i]);

      q.merge_queue(&tmq);
    };

    runConcurrentTest(mergeFun, __LINE__);
  }
}

TEST_F(MSQueueTest, concurrentPopPushTest) {
  for (unsigned int k = 0; k < 100; k++) {
    auto pushFun = [](MSQueue<int>& q, std::vector<int>& e, int i){
      e[i] = i;

      q.push_tail(e[i]);
    };

    runConcurrentTest(pushFun, __LINE__);
  }
}
