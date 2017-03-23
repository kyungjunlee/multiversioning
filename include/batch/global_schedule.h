#ifndef GLOBAL_SCHEDULE_H_
#define GLOBAL_SCHEDULE_H_

#include "batch/lock_table.h"
#include "batch/batch_action.h"

class GlobalScheduleInterface {
public:
  // scheduler thread interface:
  virtual void merge_into_global_schedule(BatchLockTable&& blt) = 0;

  // executor thread interface:
  virtual std::shared_ptr<LockStage> get_stage_holding_lock_for(
      BatchAction::RecKey key) = 0;
  virtual void finalize_execution_of_action(
      std::shared_ptr<BatchAction> act) = 0;
};

// TODO:
//    Initialization of the lock table information? Size? Etc?
//    Raw pointer to lock stage -- must be shared. Change it throughout.
class GlobalSchedule : public GlobalScheduleInterface {
protected:
  LockTable lt;

  void advance_lock_for_record(BatchAction::RecKey key);
public:
  GlobalSchedule();

  void merge_into_global_schedule(BatchLockTable&& blt) override;

  std::shared_ptr<LockStage> get_stage_holding_lock_for(
      BatchAction::RecKey key) override;
  void finalize_execution_of_action(
      std::shared_ptr<BatchAction> act) override;
};

#endif
