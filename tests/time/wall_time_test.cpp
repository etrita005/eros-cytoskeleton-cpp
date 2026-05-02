#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

TEST(WallTimeTest, NowReturnsValidTime) {
  auto now = WallTime::Now();
  EXPECT_GT(now.ToSeconds(), 0);
  EXPECT_GT(now.ToMilliseconds(), 0);
}

TEST(WallTimeTest, FromSeconds) {
  auto t = WallTime::FromSeconds(1700000000);
  EXPECT_EQ(t.ToSeconds(), 1700000000);
}

TEST(WallTimeTest, FromMilliseconds) {
  auto t = WallTime::FromMilliseconds(1700000000000LL);
  EXPECT_EQ(t.ToMilliseconds(), 1700000000000LL);
}

TEST(WallTimeTest, FromMicroseconds) {
  auto t = WallTime::FromMicroseconds(1700000000000000LL);
  EXPECT_EQ(t.ToMicroseconds(), 1700000000000000LL);
}

TEST(WallTimeTest, FromNanoseconds) {
  auto t = WallTime::FromNanoseconds(1700000000000000000LL);
  EXPECT_EQ(t.ToNanoseconds(), 1700000000000000000LL);
}

TEST(WallTimeTest, FormatDefault) {
  auto t = WallTime::FromSeconds(1700000000);
  auto formatted = t.Format();
  EXPECT_FALSE(formatted.empty());
  EXPECT_NE(formatted.find("T"), std::string::npos);
}

TEST(WallTimeTest, FormatCustom) {
  auto t = WallTime::FromSeconds(1700000000);
  auto formatted = t.Format("%Y-%m-%d");
  EXPECT_EQ(formatted.size(), 10u);
}

TEST(WallTimeTest, Addition) {
  auto t = WallTime::FromSeconds(1700000000);
  auto d = Duration::FromSeconds(3600);
  auto later = t + d;
  EXPECT_EQ(later.ToSeconds(), 1700003600);
}

TEST(WallTimeTest, Subtraction) {
  auto t1 = WallTime::FromSeconds(1700003600);
  auto t2 = WallTime::FromSeconds(1700000000);
  auto diff = t1 - t2;
  EXPECT_EQ(diff.ToSeconds(), 3600);
}

TEST(WallTimeTest, DurationSubtraction) {
  auto t = WallTime::FromSeconds(1700003600);
  auto d = Duration::FromSeconds(3600);
  auto earlier = t - d;
  EXPECT_EQ(earlier.ToSeconds(), 1700000000);
}

TEST(WallTimeTest, CompoundAddition) {
  auto t = WallTime::FromSeconds(1700000000);
  t += Duration::FromSeconds(3600);
  EXPECT_EQ(t.ToSeconds(), 1700003600);
}

TEST(WallTimeTest, CompoundSubtraction) {
  auto t = WallTime::FromSeconds(1700003600);
  t -= Duration::FromSeconds(3600);
  EXPECT_EQ(t.ToSeconds(), 1700000000);
}

TEST(WallTimeTest, Equality) {
  auto t1 = WallTime::FromSeconds(1700000000);
  auto t2 = WallTime::FromSeconds(1700000000);
  auto t3 = WallTime::FromSeconds(1700000001);
  EXPECT_TRUE(t1 == t2);
  EXPECT_FALSE(t1 == t3);
  EXPECT_TRUE(t1 != t3);
}

TEST(WallTimeTest, Comparison) {
  auto t1 = WallTime::FromSeconds(1700000000);
  auto t2 = WallTime::FromSeconds(1700000001);
  EXPECT_TRUE(t1 < t2);
  EXPECT_TRUE(t1 <= t2);
  EXPECT_TRUE(t2 > t1);
  EXPECT_TRUE(t2 >= t1);
}

TEST(WallTimeTest, FromChrono) {
  auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(1700000000));
  auto t = WallTime::FromChrono(tp);
  EXPECT_EQ(t.ToSeconds(), 1700000000);
}

TEST(WallTimeTest, ToChrono) {
  auto t = WallTime::FromSeconds(1700000000);
  auto tp = t.ToChrono();
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count(),
            1700000000);
}

TEST(WallTimeTest, ChronoRoundTrip) {
  auto original = WallTime::FromSeconds(1700000000);
  auto tp = original.ToChrono();
  auto restored = WallTime::FromChrono(tp);
  EXPECT_EQ(original.ToSeconds(), restored.ToSeconds());
}
