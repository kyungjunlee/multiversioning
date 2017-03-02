#ifndef _TEST_ACTION_H_
#define _TEST_ACTION_H_

#include <batch/batch_action.h>

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

class TestAction : public BatchAction {
private:
  RecSet writeSet;
  RecSet readSet;
public: 
  TestAction(txn* txn): BatchAction(txn) {} 

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

  // override the BatchAction functions.
  void add_read_key(RecKey rk) override {readSet.insert(rk);}
  void add_write_key(RecKey rk) override {writeSet.insert(rk);}

  uint64_t get_readset_size() const override {return readSet.size();}
  uint64_t get_writeset_size() const override {return writeSet.size();}
  RecSet* get_readset_handle() {return &readSet;}
  RecSet* get_writeset_handle() {return &writeSet;}

  // inequality calculated based on the overall number of transactions.
  bool operator<(const BatchAction& ta) const {
    return (get_readset_size() + get_writeset_size() <
      ta.get_readset_size() + ta.get_writeset_size());
  }
}; 

#endif //_TEST_ACTION_H_
