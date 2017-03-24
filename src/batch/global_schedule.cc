#include "batch/global_schedule.h"

#include <cassert>

GlobalSchedule::GlobalSchedule() {};
GlobalSchedule::GlobalSchedule(DBStorageConfig db_conf): lt(db_conf) {};

inline
void GlobalSchedule::merge_into_global_schedule(
    BatchLockTable&& blt) {
  lt.merge_batch_table(blt);
};

std::shared_ptr<LockStage> GlobalSchedule::get_stage_holding_lock_for(
    RecordKey key) {
  return lt.get_head_for_record(key);
};

void GlobalSchedule::finalize_execution_of_action(
    std::shared_ptr<IBatchAction> act) {
  auto finalize_from_set = [this, act](IBatchAction::RecordKeySet* s){
    std::shared_ptr<LockStage> ls;
    for (auto& key : *s) {
      ls = get_stage_holding_lock_for(key);

      if (ls->finalize_action(act)) {
        // if all the actions within the lockstage have finished,
        // move on and signal the next stage.
        lt.pass_lock_to_next_stage_for(key);
      }
    }   
  };

  finalize_from_set(act->get_readset_handle());
  finalize_from_set(act->get_writeset_handle());
};
