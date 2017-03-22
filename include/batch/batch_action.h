#ifndef BATCH_ACTION_H_
#define BATCH_ACTION_H_

#include "batch/batch_action_interface.h"

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

// BatchAction
//
//    Purely virtual interface of any batch action within the system.
class BatchAction : public translator {
protected:
  BatchAction(txn* t): 
    translator(t),
    action_state(static_cast<uint64_t>(BatchActionState::substantiated)),
    locks_held(0)
  {};

public:
  // typedefs
  typedef uint64_t RecKey;
  typedef std::unordered_set<RecKey> RecSet;

  uint64_t action_state;
  // changed the state of action only if current state is 
  // the "expected_state". Equivalent to CAS.
  virtual bool conditional_atomic_change_state(
      BatchActionState expected_state, 
      BatchActionState new_state) = 0;
  // change the state of the action independent of the 
  // current state. Equivalent to xchgq. Returns the old state
  // that has been changed.
  virtual BatchActionState atomic_change_state(
      BatchActionState new_state) = 0;

  uint64_t locks_held;
  virtual uint64_t notify_lock_obtained() = 0; 
  virtual bool ready_to_execute() = 0;

  virtual void add_read_key(RecKey rk) = 0;
  virtual void add_write_key(RecKey rk) = 0;
  
  virtual uint64_t get_readset_size() const = 0;
  virtual uint64_t get_writeset_size() const = 0;
  virtual RecSet* get_readset_handle() = 0;
  virtual RecSet* get_writeset_handle() = 0;

  virtual bool operator<(const BatchAction& ba2) const = 0;
};

#endif //BATCH_ACTION_H_
