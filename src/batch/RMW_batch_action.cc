#include "batch/RMW_batch_action.h"

#include <cassert>

RMWBatchAction::RMWBatchAction(txn* t) : BatchAction(t) {};

void RMWBatchAction::add_to_tmp_reads(RecordKey rk) {
  auto res = tmp_reads.emplace(std::make_pair(rk, 0)); 
  assert(res.second);
}

void RMWBatchAction::add_read_key(RecordKey rk) {
  add_to_tmp_reads(rk);

  BatchAction::add_read_key(rk);
};

void RMWBatchAction::add_write_key(RecordKey rk) {
  add_to_tmp_reads(rk);

  BatchAction::add_write_key(rk);
};

void RMWBatchAction::Run(IDBStorage* db) {
  do_reads(db);
  do_writes(db);
};

void RMWBatchAction::do_reads(IDBStorage* db) {
  auto read_for_set = [db, this](auto key_set_ptr){
    TmpReadMap::iterator it;
    for (const auto& key : *key_set_ptr) {
      it = tmp_reads.find(key);
      assert(it != tmp_reads.end());
      assert(it->second == 0);
      it->second = db->read_record_value(key);
    }
  };

  read_for_set(this->get_readset_handle());
  read_for_set(this->get_writeset_handle());
};

void RMWBatchAction::do_writes(IDBStorage* db) {
  auto write_set_handle = this->get_writeset_handle();
  TmpReadMap::iterator it;
  for (const auto& key : *write_set_handle) {
    it = tmp_reads.find(key);
    assert(it != tmp_reads.end());
    db->write_record_value(key, it->second + 1); 
  }
};
