#pragma once

#include <mutex>
#include <shared_mutex>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

class Mutex {
 public:
  Mutex() = default;
  ~Mutex() = default;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  void Lock() { mutex_.lock(); }
  void Unlock() { mutex_.unlock(); }
  bool TryLock() { return mutex_.try_lock(); }

 private:
  std::recursive_mutex mutex_;
};

class ReadWriteMutex {
 public:
  ReadWriteMutex() = default;
  ~ReadWriteMutex() = default;

  ReadWriteMutex(const ReadWriteMutex&) = delete;
  ReadWriteMutex& operator=(const ReadWriteMutex&) = delete;
  ReadWriteMutex(ReadWriteMutex&&) = delete;
  ReadWriteMutex& operator=(ReadWriteMutex&&) = delete;

  void LockRead() { mutex_.lock_shared(); }
  void UnlockRead() { mutex_.unlock_shared(); }
  void LockWrite() { mutex_.lock(); }
  void UnlockWrite() { mutex_.unlock(); }
  bool TryLockRead() { return mutex_.try_lock_shared(); }
  bool TryLockWrite() { return mutex_.try_lock(); }

 private:
  std::shared_mutex mutex_;
};

class MutexLock {
 public:
  explicit MutexLock(Mutex& mutex) : mutex_(mutex) { mutex_.Lock(); }
  ~MutexLock() { mutex_.Unlock(); }

  MutexLock(const MutexLock&) = delete;
  MutexLock& operator=(const MutexLock&) = delete;
  MutexLock(MutexLock&&) = delete;
  MutexLock& operator=(MutexLock&&) = delete;

 private:
  Mutex& mutex_;
};

class ReadLock {
 public:
  explicit ReadLock(ReadWriteMutex& mutex) : mutex_(mutex) { mutex_.LockRead(); }
  ~ReadLock() { mutex_.UnlockRead(); }

  ReadLock(const ReadLock&) = delete;
  ReadLock& operator=(const ReadLock&) = delete;
  ReadLock(ReadLock&&) = delete;
  ReadLock& operator=(ReadLock&&) = delete;

 private:
  ReadWriteMutex& mutex_;
};

class WriteLock {
 public:
  explicit WriteLock(ReadWriteMutex& mutex) : mutex_(mutex) {
    mutex_.LockWrite();
  }
  ~WriteLock() { mutex_.UnlockWrite(); }

  WriteLock(const WriteLock&) = delete;
  WriteLock& operator=(const WriteLock&) = delete;
  WriteLock(WriteLock&&) = delete;
  WriteLock& operator=(WriteLock&&) = delete;

 private:
  ReadWriteMutex& mutex_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
