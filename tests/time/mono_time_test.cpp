#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

TEST(MonoTimeTest, NowReturnsValidTime) {
  auto now = MonoTime::Now();
  (void)now;
}

TEST(MonoTimeTest, NowIsMonotonicallyIncreasing) {
  auto t1 = MonoTime::Now();
  auto t2 = MonoTime::Now();
  EXPECT_TRUE(t2 >= t1);
}

TEST(MonoTimeTest, SubtractionGivesDuration) {
  auto start = MonoTime::Now();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto end = MonoTime::Now();
  auto elapsed = end - start;
  EXPECT_GE(elapsed.ToMilliseconds(), 40);
  EXPECT_LT(elapsed.ToMilliseconds(), 200);
}

TEST(MonoTimeTest, Addition) {
  auto t = MonoTime::Now();
  auto d = Duration::FromMilliseconds(100);
  auto later = t + d;
  auto diff = later - t;
  EXPECT_EQ(diff.ToMilliseconds(), 100);
}

TEST(MonoTimeTest, SubtractionWithDuration) {
  auto t = MonoTime::Now();
  auto d = Duration::FromMilliseconds(100);
  auto earlier = t - d;
  auto diff = t - earlier;
  EXPECT_EQ(diff.ToMilliseconds(), 100);
}

TEST(MonoTimeTest, CompoundAddition) {
  auto t = MonoTime::Now();
  auto original = t;
  t += Duration::FromMilliseconds(100);
  auto diff = t - original;
  EXPECT_EQ(diff.ToMilliseconds(), 100);
}

TEST(MonoTimeTest, CompoundSubtraction) {
  auto t = MonoTime::Now();
  auto original = t;
  t -= Duration::FromMilliseconds(100);
  auto diff = original - t;
  EXPECT_EQ(diff.ToMilliseconds(), 100);
}

TEST(MonoTimeTest, Equality) {
  auto t1 = MonoTime::Now();
  auto t2 = t1;
  EXPECT_TRUE(t1 == t2);
}

TEST(MonoTimeTest, Comparison) {
  auto t1 = MonoTime::Now();
  auto t2 = t1 + Duration::FromNanoseconds(1);
  EXPECT_TRUE(t1 < t2);
  EXPECT_TRUE(t1 <= t2);
  EXPECT_TRUE(t2 > t1);
  EXPECT_TRUE(t2 >= t1);
}

TEST(MonoTimeTest, FromChrono) {
  auto tp = std::chrono::steady_clock::now();
  auto t = MonoTime::FromChrono(tp);
  (void)t;
}

TEST(MonoTimeTest, ToChrono) {
  auto t = MonoTime::Now();
  auto tp = t.ToChrono();
  auto restored = MonoTime::FromChrono(tp);
  EXPECT_TRUE(t == restored);
}

TEST(MonoTimeTest, ChronoRoundTrip) {
  auto original = MonoTime::Now();
  auto tp = original.ToChrono();
  auto restored = MonoTime::FromChrono(tp);
  EXPECT_TRUE(original == restored);
}
