#pragma once

#include <memory>

#include "cytoskeleton/concurrent/event.h"
#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace object {

class Object : public std::enable_shared_from_this<Object> {
 public:
  Object() = default;
  virtual ~Object() = default;

  Object(const Object&) = delete;
  Object& operator=(const Object&) = delete;
  Object(Object&&) = delete;
  Object& operator=(Object&&) = delete;

  std::shared_ptr<Object> GetSharedPtr() { return shared_from_this(); }

  std::shared_ptr<const Object> GetSharedPtr() const {
    return shared_from_this();
  }

  void Join() { event_.Join(); }

  bool Join(std::chrono::milliseconds timeout) {
    return event_.Join(timeout);
  }

  void Notify() { event_.Notify(); }

  void ResetNotify() { event_.Reset(); }

  concurrent::Mutex& GetMutex() { return mutex_; }

 protected:
  void Lock() { mutex_.Lock(); }
  void Unlock() { mutex_.Unlock(); }
  bool TryLock() { return mutex_.TryLock(); }

 private:
  concurrent::Mutex mutex_;
  concurrent::AutoResetEvent event_;
};

}  // namespace object
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
