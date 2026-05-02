#pragma once

#include "cytoskeleton/time/duration.h"
#include "cytoskeleton/time/wall_time.h"
#include "cytoskeleton/time/mono_time.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

class Clock {
 public:
  virtual ~Clock() = default;

  virtual WallTime GetWallTime() = 0;
  virtual MonoTime GetMonoTime() = 0;
};

class SystemClock : public Clock {
 public:
  WallTime GetWallTime() override { return WallTime::Now(); }
  MonoTime GetMonoTime() override { return MonoTime::Now(); }
};

class FakeClock : public Clock {
 public:
  FakeClock()
      : wall_time_(std::chrono::system_clock::time_point(
            std::chrono::nanoseconds(0))),
        mono_time_(std::chrono::steady_clock::time_point(
            std::chrono::nanoseconds(0))) {}

  FakeClock(WallTime wall_time, MonoTime mono_time)
      : wall_time_(wall_time.ToChrono()),
        mono_time_(mono_time.ToChrono()) {}

  WallTime GetWallTime() override {
    return WallTime::FromChrono(wall_time_);
  }

  MonoTime GetMonoTime() override {
    return MonoTime::FromChrono(mono_time_);
  }

  void Advance(Duration duration) {
    wall_time_ += duration.ToChrono();
    mono_time_ += duration.ToChrono();
  }

  void AdvanceWall(Duration duration) {
    wall_time_ += duration.ToChrono();
  }

  void AdvanceMono(Duration duration) {
    mono_time_ += duration.ToChrono();
  }

  void SetWall(WallTime wall_time) { wall_time_ = wall_time.ToChrono(); }

  void SetMono(MonoTime mono_time) { mono_time_ = mono_time.ToChrono(); }

 private:
  std::chrono::system_clock::time_point wall_time_;
  std::chrono::steady_clock::time_point mono_time_;
};

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
