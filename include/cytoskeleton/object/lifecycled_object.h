#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>

#include "cytoskeleton/concurrent/mutex.h"
#include "cytoskeleton/object/object.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace object {

class LifecycleObject : public Object {
 public:
  using Ptr = std::shared_ptr<LifecycleObject>;

  enum class State {
    kUninitialized,
    kInitializing,
    kInitialized,
    kStarting,
    kRunning,
    kStopping,
    kStopped,
    kDestroying,
    kDestroyed
  };

  LifecycleObject() = default;

  ~LifecycleObject() override {
    if (GetState() != State::kDestroyed) {
      Destroy();
    }
  }

  bool Initialize() {
    concurrent::MutexLock lock(GetMutex());
    State current = GetState();
    if (current != State::kUninitialized) {
      return current >= State::kInitialized;
    }
    SetState(State::kInitializing);
    bool result = OnInitialize();
    SetState(result ? State::kInitialized : State::kUninitialized);
    if (result) {
      OnStateChanged(State::kInitializing, State::kInitialized);
    }
    return result;
  }

  bool Start() {
    concurrent::MutexLock lock(GetMutex());
    State current = GetState();
    if (current != State::kInitialized && current != State::kStopped) {
      return current == State::kRunning || current == State::kStarting;
    }
    SetState(State::kStarting);
    bool result = OnStart();
    SetState(result ? State::kRunning : State::kStopped);
    if (result) {
      OnStateChanged(State::kStarting, State::kRunning);
    }
    return result;
  }

  bool Stop() {
    concurrent::MutexLock lock(GetMutex());
    State current = GetState();
    if (current != State::kRunning) {
      return current == State::kStopped || current == State::kStopping ||
             current == State::kInitialized;
    }
    SetState(State::kStopping);
    bool result = OnStop();
    SetState(State::kStopped);
    OnStateChanged(State::kStopping, State::kStopped);
    return result;
  }

  bool Destroy() {
    {
      concurrent::MutexLock lock(GetMutex());
      State current = GetState();
      if (current == State::kDestroyed || current == State::kDestroying) {
        return true;
      }
    }
    if (GetState() == State::kRunning) {
      Stop();
    }
    {
      concurrent::MutexLock lock(GetMutex());
      SetState(State::kDestroying);
      bool result = OnDestroy();
      SetState(State::kDestroyed);
      OnStateChanged(State::kDestroying, State::kDestroyed);
      return result;
    }
  }

  State GetState() const { return state_.load(); }

  bool IsUninitialized() const { return GetState() == State::kUninitialized; }
  bool IsInitialized() const {
    State s = GetState();
    return s >= State::kInitialized && s < State::kDestroying;
  }
  bool IsRunning() const { return GetState() == State::kRunning; }
  bool IsStopped() const { return GetState() == State::kStopped; }
  bool IsDestroyed() const { return GetState() == State::kDestroyed; }

 protected:
  virtual bool OnInitialize() { return true; }
  virtual bool OnStart() { return true; }
  virtual bool OnStop() { return true; }
  virtual bool OnDestroy() { return true; }
  virtual void OnStateChanged(State old_state, State new_state) {
    (void)old_state;
    (void)new_state;
  }

 private:
  void SetState(State state) { state_.store(state); }

  std::atomic<State> state_{State::kUninitialized};
};

}  // namespace object
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
