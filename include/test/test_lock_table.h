#ifndef TEST_LOCK_TABLE_H_
#define TEST_LOCK_TABLE_H_

#include "batch/batch_action.h"
#include "test/test_lock_stage.h"

class TestLockTable : public LockTable {
  public:
    TestLockTable(): LockTable() {};
    const LockTable::LockTableType& get_lock_table_data() {
      return lock_table;
    }

    bool lock_table_contains_stage(BatchAction::RecKey k, LockStage* ls) {
      auto lq = lock_table.find(k);
      if (lq == lock_table.end()) return false;

      // look through the whole queue to see if we actually
      // have the correct stage in the queue
      //
      // We need to use the test lock stage class to access the type
      // of the lock easily.
      TestLockStage tls(*ls);
      LockQueue::QueueElt* curr = lq->second->peek_head_elt();
      while (curr != nullptr) {
        TestLockStage ctls(*curr->get_contents());

        if (ctls.get_requesters() == tls.get_requesters() &&
            ctls.get_holders() == tls.get_holders() &&
            ctls.get_lock_type() == tls.get_lock_type()){
          return true;
        }

        curr = curr->get_next_elt();
      }

      return false;
    };
};

#endif // TEST_LOCK_TABLE_H_
