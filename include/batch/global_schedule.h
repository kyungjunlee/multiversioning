#ifndef GLOBAL_SCHEDULE_H_
#define GLOBAL_SCHEDULE_H_

#include "batch/global_schedule_interface.h"

#include <memory>

// TODO:
//    Initialization of the lock table information? Size? Etc?
// GlobalSchedule
//
//    The actual implementation of the above.
class GlobalSchedule : public GlobalScheduleInterface {
protected:
  LockTable lt;

  void advance_lock_for_record(RecordKey key);
public:
  GlobalSchedule();

  void merge_into_global_schedule(BatchLockTable&& blt) override;

  std::shared_ptr<LockStage> get_stage_holding_lock_for(
      RecordKey key) override;
  void finalize_execution_of_action(
      std::shared_ptr<BatchActionInterface> act) override;
};

#endif
