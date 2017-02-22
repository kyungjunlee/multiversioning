#ifndef _TEST_LOCK_STAGE_H_
#define _TEST_LOCK_STAGE_H_

bool operator==(const LockStage& ls1, const LockStage& ls2) {
  return (ls1.requesters == ls2.requesters &&
    ls1.holders == ls2.holders &&
    ls1.l_type == ls2.l_type);
}

/*
 * Simple Lock Stage Test fixture.
 *
 * Copies the contents of the lock stage and allows more access to the 
 * elements within LockStage.
 */
class TestLockStage : public LockStage {
public:
  TestLockStage(LockStage& ls): LockStage(ls) {};

  uint64_t get_holders() const {return holders;}
  LockType get_lock_type() const {return l_type;}
};

#endif //_TEST_LOCK_STAGE_H_
