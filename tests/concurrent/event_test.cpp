#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cytoskeleton/concurrent/event.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(ManualResetEventTest, BasicSetReset) {
  ManualResetEvent event(false);
  EXPECT_FALSE(event.IsSet());

  event.Set();
  EXPECT_TRUE(event.IsSet());

  event.Reset();
  EXPECT_FALSE(event.IsSet());
}

TEST(ManualResetEventTest, Join) {
  ManualResetEvent event(false);

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    event.Set();
  });

  event.Join();
  EXPECT_TRUE(event.IsSet());

  t.join();
}

TEST(ManualResetEventTest, JoinWithTimeoutSuccess) {
  ManualResetEvent event(false);

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    event.Set();
  });

  bool result = event.Join(std::chrono::milliseconds(500));
  EXPECT_TRUE(result);
  EXPECT_TRUE(event.IsSet());

  t.join();
}

TEST(ManualResetEventTest, JoinWithTimeoutFailure) {
  ManualResetEvent event(false);

  bool result = event.Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
  EXPECT_FALSE(event.IsSet());
}

TEST(ManualResetEventTest, MultipleWaits) {
  ManualResetEvent event(false);

  event.Set();

  EXPECT_TRUE(event.Join(std::chrono::milliseconds(10)));
  EXPECT_TRUE(event.Join(std::chrono::milliseconds(10)));
  EXPECT_TRUE(event.IsSet());
}

TEST(AutoResetEventTest, BasicSetReset) {
  AutoResetEvent event(false);
  EXPECT_FALSE(event.IsSet());

  event.Set();
  EXPECT_TRUE(event.IsSet());

  event.Reset();
  EXPECT_FALSE(event.IsSet());
}

TEST(AutoResetEventTest, AutoReset) {
  AutoResetEvent event(false);

  event.Set();
  EXPECT_TRUE(event.IsSet());

  event.Join();
  EXPECT_FALSE(event.IsSet());
}

TEST(AutoResetEventTest, JoinWithTimeoutSuccess) {
  AutoResetEvent event(false);

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    event.Set();
  });

  bool result = event.Join(std::chrono::milliseconds(500));
  EXPECT_TRUE(result);
  EXPECT_FALSE(event.IsSet());

  t.join();
}

TEST(AutoResetEventTest, JoinWithTimeoutFailure) {
  AutoResetEvent event(false);

  bool result = event.Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
  EXPECT_FALSE(event.IsSet());
}

TEST(AutoResetEventTest, MultipleSignals) {
  AutoResetEvent event(false);
  std::atomic<int> counter{0};

  std::thread t1([&]() {
    for (int i = 0; i < 3; ++i) {
      event.Join();
      ++counter;
    }
  });

  std::thread t2([&]() {
    for (int i = 0; i < 3; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      event.Set();
    }
  });

  t1.join();
  t2.join();

  EXPECT_EQ(counter, 3);
}

TEST(ManualResetEventTest, InitialStateTrue) {
  ManualResetEvent event(true);
  EXPECT_TRUE(event.IsSet());
}

TEST(AutoResetEventTest, InitialStateTrue) {
  AutoResetEvent event(true);
  EXPECT_TRUE(event.IsSet());
}

TEST(ManualResetEventTest, SpuriousWakeup) {
  ManualResetEvent event(false);
  std::atomic<bool> done{false};

  std::thread t([&]() {
    event.Join();
    done = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  event.Set();

  t.join();
  EXPECT_TRUE(done);
}
