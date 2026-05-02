#pragma once

#include "cytoskeleton/time/duration.h"
#include "cytoskeleton/time/mono_time.h"
#include "cytoskeleton/time/clock.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

class Deadline {
 public:
  static Deadline After(Duration duration) {
    return Deadline(MonoTime::Now() + duration, nullptr);
  }

  static Deadline After(Duration duration, Clock* clock) {
    auto now = clock ? clock->GetMonoTime() : MonoTime::Now();
    return Deadline(now + duration, clock);
  }

  bool IsExpired() const {
    auto now = clock_ ? clock_->GetMonoTime() : MonoTime::Now();
    return now >= expire_time_;
  }

  Duration Remaining() const {
    auto now = clock_ ? clock_->GetMonoTime() : MonoTime::Now();
    return expire_time_ - now;
  }

  MonoTime ExpireTime() const { return expire_time_; }

 private:
  Deadline(MonoTime expire_time, Clock* clock)
      : expire_time_(expire_time), clock_(clock) {}

  MonoTime expire_time_;
  Clock* clock_;
};

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
