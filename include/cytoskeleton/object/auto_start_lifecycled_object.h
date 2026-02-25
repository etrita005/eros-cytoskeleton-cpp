#pragma once

#include <chrono>
#include <concepts>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>

#include "cytoskeleton/concurrent/thread.h"
#include "cytoskeleton/object/lifecycled_object.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace object {

// Forward declaration
class AutoStartLifecycleObject;

// Concept: T must derive from AutoStartLifecycleObject
template <typename T>
concept DerivedFromAutoStartLifecycleObject = requires {
  requires std::is_base_of_v<AutoStartLifecycleObject, T>;
};

class AutoStartLifecycleObject : public LifecycleObject {
 public:
  using Ptr = std::shared_ptr<AutoStartLifecycleObject>;

  AutoStartLifecycleObject() = default;

  explicit AutoStartLifecycleObject(
      std::function<void(std::stop_token)> run_func)
      : run_func_(std::move(run_func)) {}

  // Base class LifecycleObject destructor will call Destroy()

  template <DerivedFromAutoStartLifecycleObject T, typename... Args>
  static std::shared_ptr<T> Create(Args&&... args) {
    auto obj = std::make_shared<T>(std::forward<Args>(args)...);
    if (!obj->Initialize()) {
      return nullptr;
    }
    if (!obj->Start()) {
      return nullptr;
    }
    return obj;
  }

  static std::shared_ptr<AutoStartLifecycleObject> Create(
      std::function<void(std::stop_token)> run_func) {
    auto obj = std::make_shared<AutoStartLifecycleObject>(std::move(run_func));
    if (!obj->Initialize()) {
      return nullptr;
    }
    if (!obj->Start()) {
      return nullptr;
    }
    return obj;
  }

 protected:
  virtual void Run(std::stop_token stop_token) {
    if (run_func_) {
      run_func_(stop_token);
    }
  }

  bool OnStart() override {
    if (!LifecycleObject::OnStart()) {
      return false;
    }
    thread_ = std::make_unique<concurrent::Thread>(
        "AutoStartLifecycleObject",
        [this](std::stop_token token) { Run(token); });
    thread_->Start();
    return true;
  }

  bool OnStop() override {
    if (thread_) {
      thread_->RequestStop();
      thread_->Join();
    }
    thread_.reset();
    return LifecycleObject::OnStop();
  }

 private:
  std::unique_ptr<concurrent::Thread> thread_;
  std::function<void(std::stop_token)> run_func_;
};

}  // namespace object
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
