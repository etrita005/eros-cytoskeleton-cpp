#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

TEST(StopwatchTest, BasicTiming) {
  Stopwatch sw;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto elapsed = sw.Elapsed();
  EXPECT_GE(elapsed.ToMilliseconds(), 40);
  EXPECT_LT(elapsed.ToMilliseconds(), 200);
}

TEST(StopwatchTest, ElapsedMilliseconds) {
  Stopwatch sw;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_GE(sw.ElapsedMilliseconds(), 40);
  EXPECT_LT(sw.ElapsedMilliseconds(), 200);
}

TEST(StopwatchTest, ElapsedSeconds) {
  Stopwatch sw;
  EXPECT_EQ(sw.ElapsedSeconds(), 0);
}

TEST(StopwatchTest, Reset) {
  Stopwatch sw;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  sw.Reset();
  auto elapsed = sw.Elapsed();
  EXPECT_LT(elapsed.ToMilliseconds(), 50);
}

TEST(StopwatchTest, PauseAndResume) {
  Stopwatch sw;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  sw.Pause();
  auto paused_elapsed = sw.Elapsed();
  EXPECT_TRUE(sw.IsPaused());
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto still_paused = sw.Elapsed();
  EXPECT_EQ(paused_elapsed.ToMilliseconds(), still_paused.ToMilliseconds());
  sw.Resume();
  EXPECT_FALSE(sw.IsPaused());
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto after_resume = sw.Elapsed();
  EXPECT_GT(after_resume.ToMilliseconds(), paused_elapsed.ToMilliseconds());
}

TEST(StopwatchTest, IsPausedInitially) {
  Stopwatch sw;
  EXPECT_FALSE(sw.IsPaused());
}

TEST(StopwatchTest, WithFakeClock) {
  FakeClock fake_clock;
  Stopwatch sw(&fake_clock);
  auto elapsed = sw.Elapsed();
  EXPECT_EQ(elapsed.ToNanoseconds(), 0);
  fake_clock.Advance(Duration::FromMilliseconds(100));
  elapsed = sw.Elapsed();
  EXPECT_EQ(elapsed.ToMilliseconds(), 100);
}

TEST(StopwatchTest, PauseWithFakeClock) {
  FakeClock fake_clock;
  Stopwatch sw(&fake_clock);
  fake_clock.Advance(Duration::FromMilliseconds(50));
  sw.Pause();
  fake_clock.Advance(Duration::FromMilliseconds(100));
  EXPECT_EQ(sw.ElapsedMilliseconds(), 50);
  sw.Resume();
  fake_clock.Advance(Duration::FromMilliseconds(50));
  EXPECT_EQ(sw.ElapsedMilliseconds(), 100);
}

TEST(StopwatchTest, ResetWithFakeClock) {
  FakeClock fake_clock;
  Stopwatch sw(&fake_clock);
  fake_clock.Advance(Duration::FromMilliseconds(100));
  sw.Reset();
  EXPECT_EQ(sw.ElapsedMilliseconds(), 0);
  fake_clock.Advance(Duration::FromMilliseconds(50));
  EXPECT_EQ(sw.ElapsedMilliseconds(), 50);
}
