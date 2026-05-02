#include <gtest/gtest.h>

#include <chrono>
#include <cmath>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

TEST(DurationTest, ZeroIsZero) {
  auto d = Duration::Zero();
  EXPECT_TRUE(d.IsZero());
  EXPECT_EQ(d.ToNanoseconds(), 0);
}

TEST(DurationTest, FromNanoseconds) {
  auto d = Duration::FromNanoseconds(100);
  EXPECT_EQ(d.ToNanoseconds(), 100);
}

TEST(DurationTest, FromMicroseconds) {
  auto d = Duration::FromMicroseconds(5);
  EXPECT_EQ(d.ToMicroseconds(), 5);
  EXPECT_EQ(d.ToNanoseconds(), 5000);
}

TEST(DurationTest, FromMilliseconds) {
  auto d = Duration::FromMilliseconds(500);
  EXPECT_EQ(d.ToMilliseconds(), 500);
  EXPECT_EQ(d.ToMicroseconds(), 500000);
}

TEST(DurationTest, FromSeconds) {
  auto d = Duration::FromSeconds(3);
  EXPECT_EQ(d.ToSeconds(), 3);
  EXPECT_EQ(d.ToMilliseconds(), 3000);
}

TEST(DurationTest, FromSecondsDouble) {
  auto d = Duration::FromSecondsDouble(1.5);
  EXPECT_EQ(d.ToMilliseconds(), 1500);
}

TEST(DurationTest, ToSecondsDouble) {
  auto d = Duration::FromMilliseconds(1500);
  EXPECT_NEAR(d.ToSecondsDouble(), 1.5, 1e-9);
}

TEST(DurationTest, Addition) {
  auto d1 = Duration::FromMilliseconds(500);
  auto d2 = Duration::FromSeconds(1);
  auto total = d1 + d2;
  EXPECT_EQ(total.ToMilliseconds(), 1500);
}

TEST(DurationTest, Subtraction) {
  auto d1 = Duration::FromSeconds(1);
  auto d2 = Duration::FromMilliseconds(500);
  auto diff = d1 - d2;
  EXPECT_EQ(diff.ToMilliseconds(), 500);
}

TEST(DurationTest, ScalarMultiplication) {
  auto d = Duration::FromMilliseconds(500);
  auto doubled = d * 2;
  EXPECT_EQ(doubled.ToMilliseconds(), 1000);
}

TEST(DurationTest, ScalarDivision) {
  auto d = Duration::FromMilliseconds(1000);
  auto halved = d / 2;
  EXPECT_EQ(halved.ToMilliseconds(), 500);
}

TEST(DurationTest, DurationDivision) {
  auto d1 = Duration::FromMilliseconds(500);
  auto d2 = Duration::FromMilliseconds(50);
  EXPECT_EQ(d1 / d2, 10);
}

TEST(DurationTest, Negation) {
  auto d = Duration::FromMilliseconds(100);
  auto neg = -d;
  EXPECT_TRUE(neg.IsNegative());
  EXPECT_EQ(neg.ToMilliseconds(), -100);
}

TEST(DurationTest, CompoundAddition) {
  auto d = Duration::FromMilliseconds(100);
  d += Duration::FromMilliseconds(200);
  EXPECT_EQ(d.ToMilliseconds(), 300);
}

TEST(DurationTest, CompoundSubtraction) {
  auto d = Duration::FromMilliseconds(300);
  d -= Duration::FromMilliseconds(100);
  EXPECT_EQ(d.ToMilliseconds(), 200);
}

TEST(DurationTest, Equality) {
  auto d1 = Duration::FromMilliseconds(100);
  auto d2 = Duration::FromMilliseconds(100);
  auto d3 = Duration::FromMilliseconds(200);
  EXPECT_TRUE(d1 == d2);
  EXPECT_FALSE(d1 == d3);
  EXPECT_TRUE(d1 != d3);
}

TEST(DurationTest, Comparison) {
  auto d1 = Duration::FromMilliseconds(100);
  auto d2 = Duration::FromMilliseconds(200);
  EXPECT_TRUE(d1 < d2);
  EXPECT_TRUE(d1 <= d2);
  EXPECT_TRUE(d2 > d1);
  EXPECT_TRUE(d2 >= d1);
  EXPECT_TRUE(d1 <= d1);
  EXPECT_TRUE(d1 >= d1);
}

TEST(DurationTest, IsNegative) {
  auto d = Duration::FromMilliseconds(-100);
  EXPECT_TRUE(d.IsNegative());
  EXPECT_FALSE(Duration::Zero().IsNegative());
  EXPECT_FALSE(Duration::FromMilliseconds(100).IsNegative());
}

TEST(DurationTest, Abs) {
  auto neg = Duration::FromMilliseconds(-100);
  auto pos = Duration::FromMilliseconds(100);
  EXPECT_EQ(neg.Abs().ToMilliseconds(), 100);
  EXPECT_EQ(pos.Abs().ToMilliseconds(), 100);
  EXPECT_EQ(Duration::Zero().Abs().ToMilliseconds(), 0);
}

TEST(DurationTest, FromChrono) {
  auto chrono_dur = std::chrono::seconds(5);
  auto d = Duration::FromChrono(chrono_dur);
  EXPECT_EQ(d.ToSeconds(), 5);
}

TEST(DurationTest, ToChrono) {
  auto d = Duration::FromMilliseconds(100);
  auto chrono_dur = d.ToChrono();
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(chrono_dur).count(), 100);
}

TEST(DurationTest, ChronoRoundTrip) {
  auto original = Duration::FromMilliseconds(1234);
  auto chrono_dur = original.ToChrono();
  auto restored = Duration::FromChrono(chrono_dur);
  EXPECT_EQ(original, restored);
}
