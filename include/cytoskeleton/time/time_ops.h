#pragma once

#include "cytoskeleton/time/duration.h"
#include "cytoskeleton/time/wall_time.h"
#include "cytoskeleton/time/mono_time.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

inline WallTime WallTime::operator+(const Duration& dur) const {
  return WallTime(time_point_ + dur.ToChrono());
}

inline WallTime WallTime::operator-(const Duration& dur) const {
  return WallTime(time_point_ - dur.ToChrono());
}

inline Duration WallTime::operator-(const WallTime& rhs) const {
  return Duration::FromChrono(
      std::chrono::duration_cast<std::chrono::nanoseconds>(time_point_ - rhs.time_point_));
}

inline WallTime& WallTime::operator+=(const Duration& dur) {
  time_point_ += dur.ToChrono();
  return *this;
}

inline WallTime& WallTime::operator-=(const Duration& dur) {
  time_point_ -= dur.ToChrono();
  return *this;
}

inline MonoTime MonoTime::operator+(const Duration& dur) const {
  return MonoTime(time_point_ + dur.ToChrono());
}

inline MonoTime MonoTime::operator-(const Duration& dur) const {
  return MonoTime(time_point_ - dur.ToChrono());
}

inline Duration MonoTime::operator-(const MonoTime& rhs) const {
  return Duration::FromChrono(
      std::chrono::duration_cast<std::chrono::nanoseconds>(time_point_ - rhs.time_point_));
}

inline MonoTime& MonoTime::operator+=(const Duration& dur) {
  time_point_ += dur.ToChrono();
  return *this;
}

inline MonoTime& MonoTime::operator-=(const Duration& dur) {
  time_point_ -= dur.ToChrono();
  return *this;
}

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
