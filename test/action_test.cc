#include <gtest/gtest.h>
#include <batch/batch_action.h>
#include <test/test_txn.h>

class BatchActionTest : public ::testing::Test {
protected:
  BatchAction* create_action_with_records(
      BatchAction::RecordKeySet readset, 
      BatchAction::RecordKeySet writeset) {
    BatchAction* action = new BatchAction(new TestTxn());
    for (auto it = readset.begin(); it != readset.end(); ++it) {
      action->add_read_key(*it);
    }

    for (auto it = writeset.begin(); it != writeset.end(); ++it) {
      action->add_write_key(*it);
    }
    return action;
  };
}; 

TEST(RecordKeyTest, CreateAndVerify) {
  RecordKey a(1, 2);
  ASSERT_EQ(a.key, 1);
  ASSERT_EQ(a.table_id, 2);

  RecordKey b(10);
  ASSERT_EQ(b.key, 10);
  ASSERT_EQ(b.table_id, RecordKey::DEFAULT_TABLE_ID);
}

TEST(RecordKeyTest, EqualityTest) {
  RecordKey a(1, 1);
  RecordKey b(1, 1);
  ASSERT_EQ(a, b);

  RecordKey c(1);
  RecordKey d(1);
  ASSERT_EQ(c, d);

  ASSERT_FALSE(a == c);
}

// add record keys and show readsets/writesets grow
TEST_F(BatchActionTest, AddingToActions) {
  BatchAction* a = create_action_with_records({{2,1}, {2,2}},{{10,20}});

  ASSERT_EQ(a->get_readset_size(), 2);
  ASSERT_EQ(a->get_writeset_size(), 1);
  delete a;
}

// create actions with different readset/writeset sizes and compare
TEST_F(BatchActionTest, ComparingActions) {
  BatchAction* a = create_action_with_records({1,2,3},{4,5,6});//6
  BatchAction* b = create_action_with_records({1,2,3,4,5}, {});//5
  BatchAction* c = create_action_with_records({1}, {2,3,4,5}); //5


  ASSERT_TRUE(*b < *a); //5<6
  ASSERT_TRUE(*c < *a);
  ASSERT_FALSE(*a < *b);//6<5
  ASSERT_FALSE(*a < *c);
  
  ASSERT_FALSE(*a < *a);//6<6
  ASSERT_FALSE(*b < *c);
  ASSERT_FALSE(*c < *b);
  delete a;
  delete b;
  delete c;
}

// TODO:
//    When we figure out if we need the read/write refs, this will come back
//    or get deleted.
//// create record keys with specific pointers and fetch them
//TEST_F(BatchActionTest, CheckingRefs) {
//  BatchRecord x = 10;
//  BatchRecord y = 11;
//
//  RecordKey r1(1, 0); 
//  RecordKey r2(1, 1); 
//  r1.record = &x;
//  r2.record = &y;
//
//  BatchAction* action = create_action_with_records({r1}, {r2}); 
//  
//  ASSERT_EQ(*((BatchRecord*) action->read(1, 0)), 10);
//  ASSERT_EQ(*((BatchRecord*) action->write_ref(1, 1)), 11);
//}
