#ifndef _TEST_LOCK_STAGE_H_
#define _TEST_LOCK_STAGE_H_

#include "batch/lock_stage.h"

/*
 * Simple Lock Stage Test fixture.
 *
 * Copies the contents of the lock stage and allows more access to the 
 * elements within LockStage.
 */
class TestLockStage : public LockStage {
public:
  TestLockStage(): LockStage() {};
  TestLockStage(LockStage& ls): LockStage(ls) {};

  uint64_t get_holders() const {return holders;}
  LockType get_lock_type() const {return l_type;}
};

#endif //_TEST_LOCK_STAGE_H_
