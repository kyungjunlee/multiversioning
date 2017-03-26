#ifndef TEST_LOCK_TABLE_H_
#define TEST_LOCK_TABLE_H_

#include "batch/batch_action_interface.h"
#include "test/test_lock_stage.h"

class TestLockTable : public LockTable {
  public:
    TestLockTable(): LockTable() {};
    const LockTable::LockTableType& get_lock_table_data() {
      return lock_table;
    }

    bool lock_table_contains_stage(
        RecordKey k, 
        std::shared_ptr<LockStage> ls) {
      auto lq = lock_table.find(k);
      if (lq == lock_table.end()) return false;

      // look through the whole queue to see if we actually
      // have the correct stage in the queue
      //
      // We need to use the test lock stage class to access the type
      // of the lock easily.
      LockQueue::QueueElt* curr = lq->second->peek_head_elt();
      while (curr != nullptr) {
        if (*ls == *curr->get_contents()) return true;

        curr = curr->get_next_elt();
      }

      return false;
    };
};

#endif // TEST_LOCK_TABLE_H_
