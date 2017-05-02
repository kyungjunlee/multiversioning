#include "batch/mutex_rw_guard.h"

MutexRWGuard::MutexRWGuard(pthread_rwlock_t* lck, LockType type): 
  MutexRWGuard(lck, type, false)
{};

MutexRWGuard::MutexRWGuard(pthread_rwlock_t* lck, LockType type, bool try_lock):
  mutex(lck)
{
  if (try_lock == false) {
    locked = true;
    if (type == LockType::shared) {
      read_lock();
    } else {
      write_lock();
    }
    return;
  }

  // we should only try to lock ...
  if (type == LockType::shared) {
    read_trylock();
  } else {
    write_trylock();
  }
};

void MutexRWGuard::unlock() {
  pthread_rwlock_unlock(mutex); 
  locked = false;
};

bool MutexRWGuard::write_trylock() {
  locked = (pthread_rwlock_trywrlock(mutex) == 0);
  return locked;
};

void MutexRWGuard::write_lock() {
  pthread_rwlock_wrlock(mutex);
  locked = true;
};

bool MutexRWGuard::read_trylock() {
  locked = (pthread_rwlock_tryrdlock(mutex) == 0);
  return locked;
};

void MutexRWGuard::read_lock() {
  pthread_rwlock_rdlock(mutex);
  locked = true;
};

bool MutexRWGuard::is_locked() {
  return locked;
};

MutexRWGuard::~MutexRWGuard() {
  if (locked) unlock();
};
