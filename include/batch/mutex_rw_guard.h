#ifndef MUTEX_RW_GUARD_H_
#define MUTEX_RW_GUARD_H_

#include "batch/lock_types.h"

#include <pthread.h>

// MutexRWGuard
//
//    Simple lock guard taking advantage of RAI to perform 
//    locking (and unlocking).
class MutexRWGuard {
private:
  pthread_rwlock_t* mutex;
  bool locked;
public:
  MutexRWGuard(pthread_rwlock_t* lock, LockType type);

  void unlock();
  void write_lock();
  void read_lock();

  ~MutexRWGuard();
};
#endif
