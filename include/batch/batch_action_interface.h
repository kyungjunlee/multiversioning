#ifndef BATCH_ACTION_INTERFACE_H_
#define BATCH_ACTION_INTERFACE_H_

#include "batch/record_key.h"
#include "batch/db_storage_interface.h"
#include "db.h"

#include <stdint.h>
#include <unordered_set>

enum class BatchActionState {
  // substantiated == created, but not being processed and not done
  substantiated = 0,
  // processing == claimed by an execution thread
  processing,
  // done == finished execution
  done,
  state_count
};

// IBatchAction
//
//    The general interface of an action used within our
//    system. 
class IBatchAction : public translator {
public:
  // typedefs
  typedef std::unordered_set<RecordKey> RecordKeySet;

  IBatchAction(txn* t): 
    translator(t),
    locks_held(0),
    action_state(static_cast<uint64_t>(BatchActionState::substantiated))
  {};

  uint64_t locks_held;
  virtual uint64_t notify_lock_obtained() = 0; 
  virtual bool ready_to_execute() = 0;

  virtual void add_read_key(RecordKey rk) = 0;
  virtual void add_write_key(RecordKey rk) = 0;
  
  virtual uint64_t get_readset_size() const = 0;
  virtual uint64_t get_writeset_size() const = 0;
  virtual RecordKeySet* get_readset_handle() = 0;
  virtual RecordKeySet* get_writeset_handle() = 0;

  uint64_t action_state;
  // changes the state of action only if current state is 
  // the "expected_state". Equivalent to CAS.
  virtual bool conditional_atomic_change_state(
      BatchActionState expected_state, 
      BatchActionState new_state) = 0;
  // changes the state of the action independent of the 
  // current state. Equivalent to xchgq. Returns the old state
  // that has been changed.
  virtual BatchActionState atomic_change_state(
      BatchActionState new_state) = 0;

  virtual void Run(IDBStorage* db) = 0;

  virtual bool operator<(const IBatchAction& ba2) const = 0;

  virtual ~IBatchAction() {
    delete t;
  };
};

#endif // BATCH_ACTION_INTERFACE_H_
