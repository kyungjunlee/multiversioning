#ifndef GLOBAL_SCHEDULE_INTERFACE_H_
#define GLOBAL_SCHEDULE_INTERFACE_H_

#include "batch/batch_action_interface.h"
#include "batch/lock_stage.h"
#include "batch/lock_table.h"
#include "batch/record_key.h"

#include <memory>
// IGlobalSchedule
//
//    The purely virtual interface between the global schedule and the
//    execution and scheduler thread manager.
class IGlobalSchedule {
public:
  // scheduler thread manager interface:
  virtual void merge_into_global_schedule(BatchLockTable& blt) = 0;
  // Both from and to are inclusive. Merging occurs only for records
  // in the range [from, to];
  virtual void merge_into_global_schedule_for(
      BatchLockTable& blt,
      const RecordKey& from,
      const RecordKey& to) = 0;

  // executor thread manager interface:
  virtual std::shared_ptr<LockStage> get_stage_holding_lock_for(
      RecordKey key) = 0;
  virtual void finalize_execution_of_action(IBatchAction* act) = 0;
};

#endif //GLOBAL_SCHEDULE_INTERFACE_H_
