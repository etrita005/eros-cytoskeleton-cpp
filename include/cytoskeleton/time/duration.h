#pragma once

#include <chrono>
#include <cstdint>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

class Duration {
 public:
  static Duration Zero() { return Duration(std::chrono::nanoseconds(0)); }

  static Duration FromNanoseconds(int64_t ns) {
    return Duration(std::chrono::nanoseconds(ns));
  }

  static Duration FromMicroseconds(int64_t us) {
    return Duration(std::chrono::microseconds(us));
  }

  static Duration FromMilliseconds(int64_t ms) {
    return Duration(std::chrono::milliseconds(ms));
  }

  static Duration FromSeconds(int64_t sec) {
    return Duration(std::chrono::seconds(sec));
  }

  static Duration FromSecondsDouble(double sec) {
    return Duration(std::chrono::duration<double>(sec));
  }

  static Duration FromHours(int64_t hours) {
    return Duration(std::chrono::hours(hours));
  }

  static Duration FromChrono(const std::chrono::nanoseconds& chrono_dur) {
    return Duration(chrono_dur);
  }

  int64_t ToNanoseconds() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(chrono_duration_).count();
  }

  int64_t ToMicroseconds() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(chrono_duration_).count();
  }

  int64_t ToMilliseconds() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(chrono_duration_).count();
  }

  int64_t ToSeconds() const {
    return std::chrono::duration_cast<std::chrono::seconds>(chrono_duration_).count();
  }

  double ToSecondsDouble() const {
    return std::chrono::duration<double>(chrono_duration_).count();
  }

  std::chrono::nanoseconds ToChrono() const { return chrono_duration_; }

  Duration operator+(const Duration& rhs) const {
    return Duration(chrono_duration_ + rhs.chrono_duration_);
  }

  Duration operator-(const Duration& rhs) const {
    return Duration(chrono_duration_ - rhs.chrono_duration_);
  }

  Duration operator*(int64_t scalar) const {
    return Duration(chrono_duration_ * scalar);
  }

  Duration operator/(int64_t scalar) const {
    return Duration(chrono_duration_ / scalar);
  }

  int64_t operator/(const Duration& rhs) const {
    return chrono_duration_.count() / rhs.chrono_duration_.count();
  }

  Duration operator-() const { return Duration(-chrono_duration_); }

  Duration& operator+=(const Duration& rhs) {
    chrono_duration_ += rhs.chrono_duration_;
    return *this;
  }

  Duration& operator-=(const Duration& rhs) {
    chrono_duration_ -= rhs.chrono_duration_;
    return *this;
  }

  bool operator==(const Duration& rhs) const {
    return chrono_duration_ == rhs.chrono_duration_;
  }

  bool operator!=(const Duration& rhs) const {
    return chrono_duration_ != rhs.chrono_duration_;
  }

  bool operator<(const Duration& rhs) const {
    return chrono_duration_ < rhs.chrono_duration_;
  }

  bool operator<=(const Duration& rhs) const {
    return chrono_duration_ <= rhs.chrono_duration_;
  }

  bool operator>(const Duration& rhs) const {
    return chrono_duration_ > rhs.chrono_duration_;
  }

  bool operator>=(const Duration& rhs) const {
    return chrono_duration_ >= rhs.chrono_duration_;
  }

  bool IsZero() const { return chrono_duration_.count() == 0; }

  bool IsNegative() const { return chrono_duration_.count() < 0; }

  Duration Abs() const {
    return IsNegative() ? Duration(-chrono_duration_) : Duration(chrono_duration_);
  }

 private:
  explicit Duration(std::chrono::nanoseconds ns) : chrono_duration_(ns) {}

  template <typename Rep, typename Period>
  explicit Duration(const std::chrono::duration<Rep, Period>& dur)
      : chrono_duration_(std::chrono::duration_cast<std::chrono::nanoseconds>(dur)) {}

  std::chrono::nanoseconds chrono_duration_;
};

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
