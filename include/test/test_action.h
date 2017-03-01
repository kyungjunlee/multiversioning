#ifndef _TEST_ACTION_H_
#define _TEST_ACTION_H_

#include <db.h>

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

class TestAction : public translator {
public:
  typedef uint64_t RecKey;
  typedef std::vector<RecKey> RecVec;
private:
  RecVec writeSet;
  RecVec readSet;
public: 
  TestAction(txn* txn): translator(txn) {} 

  void add_read_key(RecKey rk) {readSet.push_back(rk);}
  void add_write_key(RecKey rk) {writeSet.push_back(rk);}

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

  uint64_t get_readset_size() const {return readSet.size();}
  uint64_t get_writeset_size() const {return writeSet.size();}

  // inequality calculated based on the overall number of transactions.
  bool operator<(const TestAction& ta) const {
    return (get_readset_size() + get_writeset_size()) < 
      (ta.get_readset_size() + ta.get_writeset_size());
  }
}; 

#endif //_TEST_ACTION_H_
