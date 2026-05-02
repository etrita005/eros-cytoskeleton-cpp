#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace time {

class Duration;

class WallTime {
 public:
  static WallTime Now() {
    return WallTime(std::chrono::system_clock::now());
  }

  static WallTime FromSeconds(int64_t sec) {
    return WallTime(std::chrono::system_clock::time_point(
        std::chrono::seconds(sec) + std::chrono::system_clock::time_point::duration::zero()));
  }

  static WallTime FromMilliseconds(int64_t ms) {
    return WallTime(std::chrono::system_clock::time_point(
        std::chrono::milliseconds(ms)));
  }

  static WallTime FromMicroseconds(int64_t us) {
    return WallTime(std::chrono::system_clock::time_point(
        std::chrono::microseconds(us)));
  }

  static WallTime FromNanoseconds(int64_t ns) {
    return WallTime(std::chrono::system_clock::time_point(
        std::chrono::nanoseconds(ns)));
  }

  static WallTime FromChrono(const std::chrono::system_clock::time_point& tp) {
    return WallTime(tp);
  }

  int64_t ToSeconds() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
               time_point_.time_since_epoch())
        .count();
  }

  int64_t ToMilliseconds() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               time_point_.time_since_epoch())
        .count();
  }

  int64_t ToMicroseconds() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               time_point_.time_since_epoch())
        .count();
  }

  int64_t ToNanoseconds() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               time_point_.time_since_epoch())
        .count();
  }

  std::string Format() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(time_point_);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                  time_point_.time_since_epoch()) %
              std::chrono::seconds(1);

    std::tm tm_buf{};
    localtime_r(&time_t_val, &tm_buf);

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);

    char result[80];
    std::snprintf(result, sizeof(result), "%s.%09ld", buf,
                  static_cast<long>(ns.count()));
    return std::string(result);
  }

  std::string Format(const std::string& fmt) const {
    auto time_t_val = std::chrono::system_clock::to_time_t(time_point_);
    std::tm tm_buf{};
    localtime_r(&time_t_val, &tm_buf);

    char buf[256];
    std::strftime(buf, sizeof(buf), fmt.c_str(), &tm_buf);
    return std::string(buf);
  }

  std::chrono::system_clock::time_point ToChrono() const { return time_point_; }

  WallTime operator+(const Duration& dur) const;

  WallTime operator-(const Duration& dur) const;

  Duration operator-(const WallTime& rhs) const;

  WallTime& operator+=(const Duration& dur);

  WallTime& operator-=(const Duration& dur);

  bool operator==(const WallTime& rhs) const { return time_point_ == rhs.time_point_; }
  bool operator!=(const WallTime& rhs) const { return time_point_ != rhs.time_point_; }
  bool operator<(const WallTime& rhs) const { return time_point_ < rhs.time_point_; }
  bool operator<=(const WallTime& rhs) const { return time_point_ <= rhs.time_point_; }
  bool operator>(const WallTime& rhs) const { return time_point_ > rhs.time_point_; }
  bool operator>=(const WallTime& rhs) const { return time_point_ >= rhs.time_point_; }

 private:
  explicit WallTime(std::chrono::system_clock::time_point tp) : time_point_(tp) {}

  std::chrono::system_clock::time_point time_point_;
};

}  // namespace time
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
