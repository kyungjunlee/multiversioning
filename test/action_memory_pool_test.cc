#include "gtest/gtest.h"
#include "batch/memory_pools.h"
#include "test/test_action.h"

class ActionMemoryPoolTest : public testing::Test {
protected:
  const unsigned int txns_allocated = 100;
  std::unique_ptr<ActionMemoryPool<TestAction>> amp_ptr;

  virtual void SetUp() {
    amp_ptr = std::make_unique<ActionMemoryPool<TestAction>>();
  };
};

TEST_F(ActionMemoryPoolTest, single_allocation_test) {
  TestAction* ta = amp_ptr->alloc_and_initialize();

  // default constructed.
  ASSERT_EQ(0, ta->get_id());
  ASSERT_EQ(0, ta->get_readset_size());
  ASSERT_EQ(0, ta->get_writeset_size());
  amp_ptr->free(ta);
};

TEST_F(ActionMemoryPoolTest, allocation_mem_leak_test) {
  std::vector<TestAction*> acts;

  // we do NOT save any reference to the actions and the destructor
  // of the baa should still clear the memory correctly.
  for (unsigned int i = 0; i < txns_allocated; i++) {
    acts.push_back(amp_ptr->alloc_and_initialize());
  }

  for (auto& act : acts) {
    amp_ptr->free(act);
  }
};

TEST_F(ActionMemoryPoolTest, allocated_elts_are_unique_test) {
  TestAction* ta;
  std::unordered_set<TestAction*> acts;

  for (unsigned int i = 0; i < txns_allocated; i++) {
    ta = amp_ptr->alloc_and_initialize();
    // default constructed.
    ASSERT_EQ(0, ta->get_id());
    ASSERT_EQ(0, ta->get_readset_size());
    ASSERT_EQ(0, ta->get_writeset_size());
    acts.insert(ta);
  }

  // all of the actions are unique.
  ASSERT_EQ(acts.size(), txns_allocated);

  // free all of the memory
  for (auto& act : acts) {
    amp_ptr->free(act);
  }
};

TEST_F(ActionMemoryPoolTest, reuse_single_test) {
  TestAction* ta = amp_ptr->alloc_and_initialize();
  size_t addr_pointed_to = reinterpret_cast<std::size_t>(ta);

  for (unsigned int i = 0; i < txns_allocated; i++) {
    // the address pointed to does not change -- we reuse the memory
    ASSERT_EQ(addr_pointed_to, reinterpret_cast<std::size_t>(ta));
    amp_ptr->free(ta);
    amp_ptr->alloc_and_initialize();
  }

  amp_ptr->free(ta);
};

TEST_F(ActionMemoryPoolTest, reuse_many_test) {
  std::vector<TestAction*> acts;
  std::vector<std::size_t> addresses;
  
  for (unsigned int i = 0; i < 5; i++) {
    acts.push_back(amp_ptr->alloc_and_initialize());
    addresses.push_back(reinterpret_cast<std::size_t>(acts[i]));
  };

  unsigned int index = 0;
  for (unsigned int i = 0; i < 5 * txns_allocated; i++) {
    index = i % 5;
    ASSERT_EQ(addresses[index], reinterpret_cast<std::size_t>(acts[index]));

    amp_ptr->free(acts[index]);
    acts[index] = amp_ptr->alloc_and_initialize();
  };

  for (auto& act : acts) amp_ptr->free(act);
};
