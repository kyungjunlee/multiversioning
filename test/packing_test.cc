#include <gtest/gtest.h>
#include <batch/packing.h>
#include <batch/arr_container.h>
#include <test/test_action.h>
#include <test/test_txn.h>

#include <unordered_set>

class PackingTest : public testing::Test {
private:
  typedef IBatchAction::RecordKeySet RecordKeySet;
  std::vector<RecordKeySet> read_sets;
  std::vector<RecordKeySet> write_sets;
protected:
  std::unique_ptr<ArrayContainer> test_container;
  std::vector<IBatchAction*> allocated_actions;

  void addActionFromSets(
      RecordKeySet write_set,
      RecordKeySet read_set) {
    read_sets.push_back(read_set);
    write_sets.push_back(write_set);    
  }

  void finalizeAddingActions() {
    Container::BatchActions actions;
    
    for (unsigned int i = 0; i < read_sets.size(); i++) {
      // treat the index as id.
      TestAction* ta = new TestAction(new TestTxn(), i);
      allocated_actions.push_back(ta);

      for (auto j : read_sets[i]) ta->add_read_key(j);
      for (auto j : write_sets[i]) ta->add_write_key(j);

      actions.push_back(ta);
    }

    test_container = std::make_unique<ArrayContainer>(std::move(actions));
  }

  std::unordered_set<uint64_t> collect_ids(
      const Container::BatchActions &packing) {
    std::unordered_set<uint64_t> ids;
    
    for (const auto& j : packing) {
      // this is safe since we know we are dealing with TestActions!
      ids.insert(static_cast<TestAction*>(j)->get_id());
    }

    return ids;
  }

  void assertPackings(std::vector<std::unordered_set<uint64_t>> expected) {
    for (auto& j : expected) {
      auto packing = Packer::get_packing(test_container.get());
      std::unordered_set<uint64_t> ids = collect_ids(packing);
      EXPECT_EQ(j, ids);

      test_container->sort_remaining();
    }
  }

  virtual void SetUp() {} 

  virtual void TearDown() {
    for (auto& act_ptr : allocated_actions) {
      delete act_ptr;
    }
  }
};

// Input, all exclusive:
//    T0: 1
//    T1: 1, 3, 4
//    T2: 2, 3
// Correct packings would be:
//    1)  T0, T2
//    2)  T1
TEST_F(PackingTest, smallestExclusiveResult) {
  addActionFromSets({1}, {}); 
  addActionFromSets({1, 3, 4}, {}); 
  addActionFromSets({2, 3}, {});
  finalizeAddingActions();

  assertPackings({{0, 2}, {1}});
}  

// Input, all exclusive:
//    T0: 1
//    T1: 2, 3, 4, 5
//    T2: 1, 2, 4
// Correct packing: 
//    1) T0, T1 
//    2) T2
TEST_F(PackingTest, smallestLargestExclusiveResult) {
  addActionFromSets({1}, {}); 
  addActionFromSets({2, 3, 4, 5}, {}); 
  addActionFromSets({1, 2, 4}, {});
  finalizeAddingActions();

  assertPackings({{0, 1}, {2}});
}

// Input, all shared:
//    T0: 1, 3
//    T1: 1, 2
//    T2: 2, 3
// Correct packing: 
//    1) T0, T1, T2 
TEST_F(PackingTest, smallSharedOnlyResult) {
  addActionFromSets({}, {1, 3});
  addActionFromSets({}, {1, 2});
  addActionFromSets({}, {2, 3});
  finalizeAddingActions();

  assertPackings({{0, 1, 2}});
}

// Input, mixed:
//  T0: 1s, 2
//  T1: 1s, 3
//  T2: 1s, 2s, 3s
// Correct packing:
//    1) T0, T1
//    2) T2
TEST_F(PackingTest, smallMixedResult) {
  addActionFromSets({2}, {1});
  addActionFromSets({3}, {1});
  addActionFromSets({}, {1, 2, 3});
  finalizeAddingActions();

  assertPackings({{0, 1}, {2}});
}

// TODO: more tests with mixed cases
