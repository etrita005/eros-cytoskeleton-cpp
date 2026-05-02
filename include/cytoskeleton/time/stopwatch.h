#pragma once

#include "cytoskeleton/time/duration.h"
#include "cytoskeleton/time/mono_time.h"
#include "cytoskeleton/time/clock.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

class Stopwatch {
 public:
  Stopwatch()
      : clock_(nullptr),
        start_(MonoTime::Now()),
        paused_(false),
        paused_elapsed_(Duration::Zero()) {}

  explicit Stopwatch(Clock* clock)
      : clock_(clock),
        start_(clock ? clock->GetMonoTime() : MonoTime::Now()),
        paused_(false),
        paused_elapsed_(Duration::Zero()) {}

  void Reset() {
    start_ = clock_ ? clock_->GetMonoTime() : MonoTime::Now();
    paused_elapsed_ = Duration::Zero();
    paused_ = false;
  }

  void Pause() {
    if (!paused_) {
      paused_elapsed_ = Elapsed();
      paused_ = true;
    }
  }

  void Resume() {
    if (paused_) {
      start_ = clock_ ? clock_->GetMonoTime() : MonoTime::Now();
      paused_ = false;
    }
  }

  Duration Elapsed() const {
    if (paused_) {
      return paused_elapsed_;
    }
    auto now = clock_ ? clock_->GetMonoTime() : MonoTime::Now();
    return paused_elapsed_ + (now - start_);
  }

  int64_t ElapsedNanoseconds() const { return Elapsed().ToNanoseconds(); }
  int64_t ElapsedMicroseconds() const { return Elapsed().ToMicroseconds(); }
  int64_t ElapsedMilliseconds() const { return Elapsed().ToMilliseconds(); }
  int64_t ElapsedSeconds() const { return Elapsed().ToSeconds(); }

  bool IsPaused() const { return paused_; }

 private:
  Clock* clock_;
  MonoTime start_;
  bool paused_;
  Duration paused_elapsed_;
};

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
