#include "batch/RMW_batch_action.h"

#include <cassert>

bool RMWBatchAction::TmpRead::operator==(const TmpRead &other) const {
  return rec_key == other.rec_key;
};

bool RMWBatchAction::TmpRead::operator<(const TmpRead &other) const {
  return rec_key < other.rec_key;
};

bool RMWBatchAction::TmpRead::operator>(const TmpRead &other) const {
  return rec_key > other.rec_key;
};

RMWBatchAction::RMWBatchAction(txn* t) : BatchAction(t) {};

void RMWBatchAction::add_to_tmp_reads(RecordKey rk) {
  assert(tmp_reads.contains({rk}) == false);
  auto res = tmp_reads.insert({rk}); 
  assert(res);
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
  assert(tmp_reads.size() == this->get_readset_size() + this->get_writeset_size());
  for (auto& tmp_struct : tmp_reads) {
    tmp_struct.rec_val = db->read_record_value(tmp_struct.rec_key);
  } 
};

void RMWBatchAction::do_writes(IDBStorage* db) {
  auto write_set_handle = this->get_writeset_handle();
  for (const auto& key : *write_set_handle) {
    auto elt = tmp_reads.find({key});
    assert(elt != nullptr);
    db->write_record_value(key, elt->rec_val + 1); 
  }
};
