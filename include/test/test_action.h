#ifndef _TEST_ACTION_H_
#define _TEST_ACTION_H_

#include <batch/batch_action_interface.h>
#include <test/test_txn.h>
#include "util.h"

/*
 * Simple test fixture for actions.
 *
 * NOTE: We don't need most of the functionality offered by the actions and
 * translators. All we need is the existance of read and write sets!
 *
 * TODO:
 *    Make plugging new comparison operators easy by making the one below
 *    just a declaration of a static function!
 */

class TestAction : public IBatchAction {
private:
  RecordKeySet writeSet;
  RecordKeySet readSet;
  uint64_t id;
  
public: 
  TestAction(txn* txn): TestAction(txn, 0) {} 
  TestAction(txn* txn, uint64_t id): TestAction(txn, {}, {}, id) {}
  TestAction(
      txn* txn, 
      RecordKeySet writes,
      RecordKeySet reads, 
      uint64_t id = 0) :
    IBatchAction(txn),
    writeSet(writes),
    readSet(reads),
    id(id)
  {};

  // override the translator functions
  void *write_ref(uint64_t key, uint32_t table) override {
    // suppress "unused parameter"
    (void)(key);
    (void)(table);
    return nullptr;}
  void *read(uint64_t key, uint32_t table) override {
    // suppress "unused parameter"
    (void)(key);
    (void)(table);
    return nullptr;}
  int rand() override {return 0;}

  // state functions
  bool conditional_atomic_change_state(
      BatchActionState expected_state,
      BatchActionState new_state) override {
    return cmp_and_swap(
      &action_state,
      static_cast<uint64_t>(expected_state),
      static_cast<uint64_t>(new_state));
  }
  BatchActionState atomic_change_state(
      BatchActionState new_state) override {
    return static_cast<BatchActionState>(
      xchgq(&action_state, static_cast<uint64_t>(new_state)));
  }

  // locks_held functions
  uint64_t notify_lock_obtained() {
    return fetch_and_increment(&locks_held);
  }
  bool ready_to_execute() {
    uint64_t l = locks_held;
    barrier();
    return l == get_readset_size() + get_writeset_size(); 
  };

  // key fnuctions
  void add_read_key(RecordKey rk) override {readSet.insert(rk);}
  void add_write_key(RecordKey rk) override {writeSet.insert(rk);}

  // read/write set functions
  uint64_t get_readset_size() const override {return readSet.size();}
  uint64_t get_writeset_size() const override {return writeSet.size();}
  RecordKeySet* get_readset_handle() {return &readSet;}
  RecordKeySet* get_writeset_handle() {return &writeSet;}

  void Run(IDBStorage* db) override {(void) db;};
  // inequality calculated based on the overall number of transactions.
  bool operator<(const IBatchAction& ta) const {
    return (get_readset_size() + get_writeset_size() <
      ta.get_readset_size() + ta.get_writeset_size());
  }

  // own functions
  uint64_t get_id() const {return id;}
  static TestAction* make_test_action_with_test_txn(
      RecordKeySet writes,
      RecordKeySet reads,
      uint64_t id = 0) {
    TestAction* ta = new TestAction(new TestTxn());

    for (auto& j : writes) ta->add_write_key(j);
    for (auto& j : reads) ta->add_read_key(j);
    ta->id = id;

    return ta;
  };

  virtual ~TestAction(){};
}; 

#endif //_TEST_ACTION_H_
