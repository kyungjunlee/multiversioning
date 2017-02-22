#include <gtest/gtest.h>
#include <batch/packing.h>
#include <batch/arr_container.h>
#include <test/test_action.h>
#include <test/test_txn.h>

#include <unordered_set>

class PackingTest : public testing::Test {
private:
  std::vector<TestAction::RecSet> readSets;
  std::vector<TestAction::RecSet> writeSets;
protected:
  std::unique_ptr<ArrayContainer> testContainer;

  void addActionFromSets(
      TestAction::RecSet writeSet,
      TestAction::RecSet readSet) {
    readSets.push_back(readSet);
    writeSets.push_back(writeSet);    
  }

  void finalizeAddingActions() {
    std::unique_ptr<ArrayContainer::actions_vec> actions 
      = std::make_unique<ArrayContainer::actions_vec>();
    
    for (unsigned int i = 0; i < readSets.size(); i++) {
      // treat the index as id.
      std::unique_ptr<TestAction> ta = std::make_unique<TestAction>(new TestTxn(), i);

      for (auto j : readSets[i]) ta->add_read_key(j);
      for (auto j : writeSets[i]) ta->add_write_key(j);

      actions->push_back(std::move(ta));
    }

    testContainer = std::make_unique<ArrayContainer>(std::move(actions));
  }

  std::unordered_set<uint64_t> collect_ids(
      const Packer::ActionUptVector &packing) {
    std::unordered_set<uint64_t> ids;
    
    for (const auto& j : packing) {
      // this is safe since we know we are dealing with TestActions!
      ids.insert(
          dynamic_cast<TestAction*>(j.get())->get_id());
    }

    return ids;
  }

  void assertPackings(std::vector<std::unordered_set<uint64_t>> expected) {
    for (auto& j : expected) {
      auto packing = Packer::get_packing(testContainer.get());
      std::unordered_set<uint64_t> ids = collect_ids(packing);
      EXPECT_EQ(j, ids);

      testContainer->sort_remaining();
    }
  }

  virtual void SetUp() {} 
};

// 1 <- T0 <- T1
// 2 <- T2
// 3 <- T2 <- T1
// 4 <- T1
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

// 1 <- T0 <- T2
// 2 <- T1 <- T2
// 3 <- T1
// 4 <- T2 <- T1
// 5 <- T1
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

// 1 <- T0s <- T1s
// 2 <- T2s <- T1s
// 3 <- T0s <- T2s
// Correct packing: 
//    1) T0, T1, T2 
TEST_F(PackingTest, smallSharedOnlyResult) {
  addActionFromSets({}, {1, 3});
  addActionFromSets({}, {1, 2});
  addActionFromSets({}, {2, 3});
  finalizeAddingActions();

  assertPackings({{0, 1, 2}});
}

// 1 <- T0s <- T1s <- T2s
// 2 <- T0 <- T2s
// 3 <- T1 <- T2s
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
