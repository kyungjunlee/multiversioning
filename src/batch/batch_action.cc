#include "batch/batch_action.h"
#include "util.h"
#include <cassert>

uint64_t BatchAction::get_readset_size() const {
  return readset.size();
}

uint64_t BatchAction::get_writeset_size() const {
  return writeset.size();
}

void BatchAction::add_write_key(RecordKey rk) {
  writeset.insert(rk);
}

void BatchAction::add_read_key(RecordKey rk) {
  readset.insert(rk);
}

void* BatchAction::write_ref(uint64_t key, uint32_t table) {
  RecordKey rk(key, table);
  auto it = writeset.find(rk);
  assert(it != writeset.end());//record must be part of this action
  // TODO:
  //    Do we need this anywhere?
  //return it->record;
  return nullptr;
}

void* BatchAction::read(uint64_t key, uint32_t table) {
  RecordKey rk(key, table);
  auto it = readset.find(rk);
  assert(it != readset.end());
  // TODO:
  //    Do we need this anywhere?
  // return it->record;
  return nullptr;
};

uint64_t BatchAction::notify_lock_obtained() {
  return fetch_and_increment(&locks_held);
}

bool BatchAction::ready_to_execute() {
  uint64_t l = locks_held;
  barrier();
  return l == get_readset_size() + get_writeset_size(); 
};

IBatchAction::RecordKeySet* BatchAction::get_readset_handle() {
  return &readset;
}

IBatchAction::RecordKeySet* BatchAction::get_writeset_handle() {
  return &writeset;
}

bool BatchAction::conditional_atomic_change_state(
    BatchActionState expected_state,
    BatchActionState new_state) {
  return cmp_and_swap(
    &action_state,
    static_cast<uint64_t>(expected_state),
    static_cast<uint64_t>(new_state));
}

BatchActionState BatchAction::atomic_change_state(
      BatchActionState new_state) {
  return static_cast<BatchActionState>(
    xchgq(&action_state, static_cast<uint64_t>(new_state)));
}

//currently sort by total number of records in action
bool BatchAction::operator<(const IBatchAction& ba2) const {
  return (get_readset_size() + get_writeset_size() < 
          ba2.get_readset_size() + ba2.get_writeset_size());
}

int BatchAction::rand() {
  return 0;
}

void BatchAction::Run(IDBStorage* db) {
  (void) db;
}
