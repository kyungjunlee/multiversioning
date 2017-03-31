#include "gtest/gtest.h"
#include "batch/SPSC_MR_queue.h"

#include <vector>
#include <thread>

// NOTE:
//    All of the tests of SPSCMRQueues are done using integers for simplicity.

class SPSCMRQueueTest : public testing::Test {
protected:
  std::shared_ptr<SPSCMRQueue<int>> make_queue(int n, int start_elt = 0) {
    std::shared_ptr<SPSCMRQueue<int>> lq = std::make_shared<SPSCMRQueue<int>>();
    for (int i = start_elt; i < start_elt + n; i++) {
      lq->push_tail(i);
    }
    
    return lq;
  }
};

TEST_F(SPSCMRQueueTest, constructorTest) {
  SPSCMRQueue<int> lq;
  ASSERT_TRUE(lq.is_empty());
} 

TEST_F(SPSCMRQueueTest, push_tailNonConcurrentTest) {
  SPSCMRQueue<int> tmq;
  std::vector<int> elts;
  for (int i = 0; i < 100; i ++) {
    elts.push_back(i);
    tmq.push_tail(elts[i]);

    // the tail moves while the head remains.
    ASSERT_EQ(elts[i], *tmq.peek_tail());
    ASSERT_EQ(elts[0], *tmq.peek_head());
  }

  // The queue is well formed.
  auto curr = tmq.peek_head_elt();
  for (int i = 0; i < 100; i++) {
    if (i != 99) ASSERT_EQ(elts[i+1], *curr->get_next_elt()->get_contents());

    curr = curr->get_next_elt();
  }

  ASSERT_EQ(nullptr, tmq.peek_tail_elt()->get_next_elt());
}

TEST_F(SPSCMRQueueTest, merge_queueNonconcurrentTest) {
  auto q = make_queue(100);
  std::shared_ptr<SPSCMRQueue<int>> spscmrQueue = std::make_shared<SPSCMRQueue<int>>();

  auto checkQueue = [](auto head_elt, int elts){
    auto curr = head_elt;
    for (unsigned int i = 0; i < elts; i++) {
      if (i != elts-1) ASSERT_EQ(i, *curr->get_contents());

      curr = curr->get_next_elt();
    }
  };

  spscmrQueue->merge_queue(q.get());

  // should contain all of the elts there 
  ASSERT_EQ(0, *spscmrQueue->peek_head());
  ASSERT_EQ(99, *spscmrQueue->peek_tail());
  checkQueue(spscmrQueue->peek_head_elt(), 100);

  // repeated merging should leave the former elements unchanged and add more elements.
  q = make_queue(100, 100);
  spscmrQueue->merge_queue(q.get());
  
  ASSERT_EQ(0, *spscmrQueue->peek_head());
  ASSERT_EQ(199, *spscmrQueue->peek_tail());
  checkQueue(spscmrQueue->peek_head_elt(), 200);
}

TEST_F(SPSCMRQueueTest, pop_headNonconcurrentTest) {
  auto q = make_queue(2);

  auto head = q->peek_head_elt();
  auto tail = q->peek_tail_elt();
  auto next_head = head->get_next_elt();

  ASSERT_EQ(0, *q->peek_head());
  q->pop_head();
  ASSERT_EQ(next_head, q->peek_head_elt());
  ASSERT_EQ(tail, q->peek_tail_elt());

  ASSERT_EQ(1, *q->peek_head());
  q->pop_head();
  ASSERT_EQ(nullptr, q->peek_head_elt());
  ASSERT_EQ(nullptr, q->peek_tail_elt());
}

typedef 
    std::function<void (SPSCMRQueue<int>&, std::vector<int>&, int)> 
    pushFunType;
void runConcurrentTest(pushFunType pushFun, unsigned int line_num) {
  unsigned int elts_count = 100; 
  std::thread threads[2];
  std::vector<int> elts(elts_count);
  SPSCMRQueue<int> q;

  // create the producer
  threads[0] = std::thread([&q, &elts, elts_count, &pushFun](){
    for (unsigned int i = 0; i < elts_count; i++) {
      pushFun(q, elts, i);
    }
  }); 
  // create the consumer
  threads[1] = std::thread([&q, &elts, elts_count, &line_num](){
    std::shared_ptr<int> currDeqInt;
    for (unsigned int i = 0; i < elts_count; i++) {
      // make sure we dequeu
      while (q.is_empty());
      currDeqInt = q.peek_head();

      // consistent values from dequeueing
      ASSERT_EQ(elts[i], *currDeqInt) << 
        "From test beginning on line" << line_num;
      q.pop_head();
    }
  });

  threads[0].join();
  threads[1].join();

  // the queue should be empty by now
  ASSERT_TRUE(q.is_empty());
  ASSERT_EQ(nullptr, q.peek_head_elt()) <<
    "From test beginning on line" << line_num;
  ASSERT_EQ(nullptr, q.peek_tail_elt()) <<
    "From test beginning on line" << line_num;
};

TEST_F(SPSCMRQueueTest, concurrentPopMergeTest) {
  for (unsigned int k = 0; k < 100; k++) {
    auto mergeFun = [](SPSCMRQueue<int>& q, std::vector<int>& e, int i){
      e[i] = i;
      SPSCMRQueue<int> tmq;
      tmq.push_tail(e[i]);

      q.merge_queue(&tmq);
    };

    runConcurrentTest(mergeFun, __LINE__);
  }
}

TEST_F(SPSCMRQueueTest, concurrentPopPushTest) {
  for (unsigned int k = 0; k < 100; k++) {
    auto pushFun = [](SPSCMRQueue<int>& q, std::vector<int>& e, int i){
      e[i] = i;

      q.push_tail(e[i]);
    };

    runConcurrentTest(pushFun, __LINE__);
  }
}
