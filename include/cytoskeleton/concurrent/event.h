#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

class Event {
 public:
  explicit Event(bool initial_state = false) : signaled_(initial_state) {}
  virtual ~Event() = default;

  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;
  Event(Event&&) = delete;
  Event& operator=(Event&&) = delete;

  void Notify() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      signaled_ = true;
    }
    cv_.notify_all();
  }

  virtual void Reset() = 0;

  void Join() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return signaled_.load(); });
  }

  bool Join(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, timeout,
                        [this] { return signaled_.load(); });
  }

  bool IsNotified() const { return signaled_.load(); }

 protected:
  std::atomic<bool> signaled_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

class AutoResetEvent : public Event {
 public:
  explicit AutoResetEvent(bool initial_state = false)
      : Event(initial_state) {}

  void Reset() override {
    std::lock_guard<std::mutex> lock(mutex_);
    signaled_ = false;
  }

  void Join() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return signaled_.load(); });
    signaled_ = false;
  }

  bool Join(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool result =
        cv_.wait_for(lock, timeout, [this] { return signaled_.load(); });
    if (result) {
      signaled_ = false;
    }
    return result;
  }
};

class ManualResetEvent : public Event {
 public:
  explicit ManualResetEvent(bool initial_state = false)
      : Event(initial_state) {}

  void Reset() override {
    std::lock_guard<std::mutex> lock(mutex_);
    signaled_ = false;
  }
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
