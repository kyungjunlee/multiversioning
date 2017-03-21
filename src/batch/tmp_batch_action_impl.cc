#include "util.h"

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
    xchgq(&action_state, static_cast<uint64_t>(new_state));
  );
}

uint64_t BatchAction::notify_lock_obtained() {
  return fetch_and_increment(&locks_held);
}

bool BatchAction::ready_to_execute() {
  uint64_t l = locks_held;
  barrier();
  return l == get_readset_size() + get_writeset_size(); 
};
