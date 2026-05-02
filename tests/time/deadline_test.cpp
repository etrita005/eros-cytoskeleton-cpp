#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

TEST(DeadlineTest, NotExpiredInitially) {
  auto deadline = Deadline::After(Duration::FromSeconds(10));
  EXPECT_FALSE(deadline.IsExpired());
}

TEST(DeadlineTest, ExpiredAfterDuration) {
  auto deadline = Deadline::After(Duration::FromMilliseconds(10));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(deadline.IsExpired());
}

TEST(DeadlineTest, RemainingIsPositive) {
  auto deadline = Deadline::After(Duration::FromSeconds(10));
  auto remaining = deadline.Remaining();
  EXPECT_GT(remaining.ToSeconds(), 0);
}

TEST(DeadlineTest, RemainingIsNegativeAfterExpiry) {
  auto deadline = Deadline::After(Duration::FromMilliseconds(10));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto remaining = deadline.Remaining();
  EXPECT_TRUE(remaining.IsNegative());
}

TEST(DeadlineTest, ExpireTime) {
  auto before = MonoTime::Now();
  auto deadline = Deadline::After(Duration::FromSeconds(5));
  auto after = MonoTime::Now();
  auto expire_time = deadline.ExpireTime();
  EXPECT_TRUE(expire_time >= before + Duration::FromSeconds(5));
  EXPECT_TRUE(expire_time <= after + Duration::FromSeconds(5));
}

TEST(DeadlineTest, WithFakeClock) {
  FakeClock fake_clock;
  auto deadline = Deadline::After(Duration::FromSeconds(5), &fake_clock);
  EXPECT_FALSE(deadline.IsExpired());
  EXPECT_GT(deadline.Remaining().ToSeconds(), 0);
  fake_clock.Advance(Duration::FromSeconds(6));
  EXPECT_TRUE(deadline.IsExpired());
  EXPECT_TRUE(deadline.Remaining().IsNegative());
}

TEST(DeadlineTest, WithFakeClockExactExpiry) {
  FakeClock fake_clock;
  auto deadline = Deadline::After(Duration::FromSeconds(5), &fake_clock);
  fake_clock.Advance(Duration::FromSeconds(5));
  EXPECT_TRUE(deadline.IsExpired());
}

TEST(DeadlineTest, ZeroDeadlineIsExpired) {
  auto deadline = Deadline::After(Duration::Zero());
  EXPECT_TRUE(deadline.IsExpired());
}
