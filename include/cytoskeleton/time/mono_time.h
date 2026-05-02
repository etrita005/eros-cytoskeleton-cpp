#pragma once

#include <chrono>
#include <cstdint>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

class Duration;

class MonoTime {
 public:
  static MonoTime Now() {
    return MonoTime(std::chrono::steady_clock::now());
  }

  static MonoTime FromChrono(const std::chrono::steady_clock::time_point& tp) {
    return MonoTime(tp);
  }

  std::chrono::steady_clock::time_point ToChrono() const { return time_point_; }

  MonoTime operator+(const Duration& dur) const;

  MonoTime operator-(const Duration& dur) const;

  Duration operator-(const MonoTime& rhs) const;

  MonoTime& operator+=(const Duration& dur);

  MonoTime& operator-=(const Duration& dur);

  bool operator==(const MonoTime& rhs) const { return time_point_ == rhs.time_point_; }
  bool operator!=(const MonoTime& rhs) const { return time_point_ != rhs.time_point_; }
  bool operator<(const MonoTime& rhs) const { return time_point_ < rhs.time_point_; }
  bool operator<=(const MonoTime& rhs) const { return time_point_ <= rhs.time_point_; }
  bool operator>(const MonoTime& rhs) const { return time_point_ > rhs.time_point_; }
  bool operator>=(const MonoTime& rhs) const { return time_point_ >= rhs.time_point_; }

 private:
  explicit MonoTime(std::chrono::steady_clock::time_point tp) : time_point_(tp) {}

  std::chrono::steady_clock::time_point time_point_;
};

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
