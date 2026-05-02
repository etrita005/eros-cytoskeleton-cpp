#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

TEST(ClockTest, SystemClockReturnsValidTime) {
  SystemClock clock;
  auto wall = clock.GetWallTime();
  auto mono = clock.GetMonoTime();
  EXPECT_GT(wall.ToSeconds(), 0);
  (void)mono;
}

TEST(ClockTest, SystemClockWallTimeMatchesNow) {
  SystemClock clock;
  auto from_clock = clock.GetWallTime();
  auto from_now = WallTime::Now();
  auto diff = from_now - from_clock;
  EXPECT_LT(diff.Abs().ToSeconds(), 2);
}

TEST(ClockTest, FakeClockInitialTimeIsEpoch) {
  FakeClock clock;
  auto wall = clock.GetWallTime();
  auto mono = clock.GetMonoTime();
  EXPECT_EQ(wall.ToNanoseconds(), 0);
  EXPECT_EQ(mono.ToChrono().time_since_epoch().count(), 0);
}

TEST(ClockTest, FakeClockAdvanceWallAndMono) {
  FakeClock clock;
  clock.Advance(Duration::FromSeconds(10));
  auto wall = clock.GetWallTime();
  auto mono = clock.GetMonoTime();
  EXPECT_EQ(wall.ToSeconds(), 10);
  EXPECT_EQ((mono - MonoTime::FromChrono(std::chrono::steady_clock::time_point(std::chrono::nanoseconds(0)))).ToSeconds(), 10);
}

TEST(ClockTest, FakeClockAdvanceWallOnly) {
  FakeClock clock;
  clock.AdvanceWall(Duration::FromSeconds(10));
  auto wall = clock.GetWallTime();
  auto mono = clock.GetMonoTime();
  EXPECT_EQ(wall.ToSeconds(), 10);
  EXPECT_EQ(mono.ToChrono().time_since_epoch().count(), 0);
}

TEST(ClockTest, FakeClockAdvanceMonoOnly) {
  FakeClock clock;
  clock.AdvanceMono(Duration::FromSeconds(10));
  auto wall = clock.GetWallTime();
  auto mono = clock.GetMonoTime();
  EXPECT_EQ(wall.ToNanoseconds(), 0);
  auto mono_elapsed = mono - MonoTime::FromChrono(std::chrono::steady_clock::time_point(std::chrono::nanoseconds(0)));
  EXPECT_EQ(mono_elapsed.ToSeconds(), 10);
}

TEST(ClockTest, FakeClockSetWall) {
  FakeClock clock;
  auto target = WallTime::FromSeconds(1700000000);
  clock.SetWall(target);
  auto wall = clock.GetWallTime();
  EXPECT_EQ(wall.ToSeconds(), 1700000000);
}

TEST(ClockTest, FakeClockSetMono) {
  FakeClock clock;
  auto start = clock.GetMonoTime();
  clock.AdvanceMono(Duration::FromSeconds(5));
  auto after = clock.GetMonoTime();
  auto diff = after - start;
  EXPECT_EQ(diff.ToSeconds(), 5);
}

TEST(ClockTest, FakeClockConstructedWithTime) {
  auto wall = WallTime::FromSeconds(1700000000);
  auto mono = MonoTime::FromChrono(std::chrono::steady_clock::time_point(std::chrono::nanoseconds(1000000000)));
  FakeClock clock(wall, mono);
  EXPECT_EQ(clock.GetWallTime().ToSeconds(), 1700000000);
}

TEST(ClockTest, ClockPolymorphism) {
  FakeClock fake_clock;
  Clock* clock = &fake_clock;
  fake_clock.Advance(Duration::FromSeconds(10));
  auto wall = clock->GetWallTime();
  EXPECT_EQ(wall.ToSeconds(), 10);
}
