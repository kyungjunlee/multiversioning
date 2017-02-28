#ifndef _TEST_TXN_H_
#define _TEST_TXN_H_

#include <db.h>
#include <stdint.h>
#include <vector>

/*
 * Simple test fixture implementing the txn interface.
 *
 * NOTE: for now we don't need any of the functionality of 
 *    txn really. 
 */
class TestTxn : public txn {
public:
  TestTxn() {};
  bool Run() override {return true;};
  uint32_t num_reads() override {return 0;};
  uint32_t num_writes() override {return 0;};
};

#endif
