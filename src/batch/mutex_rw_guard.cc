#include "batch/mutex_rw_guard.h"

MutexRWGuard::MutexRWGuard(pthread_rwlock_t* lck, LockType type):
  locked(true)
{
  mutex = lck;
  if (type == LockType::shared) {
    read_lock();
  } else {
    write_lock();
  }
}

inline
void MutexRWGuard::unlock() {
  pthread_rwlock_unlock(mutex); 
  locked = false;
}

inline
void MutexRWGuard::write_lock() {
  pthread_rwlock_wrlock(mutex);
  locked = true;
}

inline
void MutexRWGuard::read_lock() {
  pthread_rwlock_rdlock(mutex);
  locked = true;
}

MutexRWGuard::~MutexRWGuard() {
  if (locked) unlock();
}
