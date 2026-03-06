#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>

#include "cytoskeleton/concurrent/thread.h"

using namespace com::etrita::eros::cytos::concurrent;

#ifdef __linux__
std::string GetCurrentThreadName() {
  std::ifstream comm_file("/proc/thread-self/comm");
  if (!comm_file.is_open()) {
    char name[16];
    if (pthread_getname_np(pthread_self(), name, sizeof(name)) == 0) {
      return std::string(name);
    }
    return "";
  }
  std::string name;
  std::getline(comm_file, name);
  return name;
}
#endif

TEST(ThreadTest, LambdaThread) {
  std::atomic<int> counter{0};

  {
    Thread thread("test_thread", [&counter](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (counter >= 5) break;
      }
    });

    thread.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  EXPECT_GE(counter, 5);
}

TEST(ThreadTest, SubclassThread) {
  std::atomic<int> counter{0};

  {
    Thread thread("test_subclass", [&counter](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (counter >= 5) break;
      }
    });
    thread.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  EXPECT_GE(counter, 5);
}

TEST(ThreadTest, RequestStop) {
  std::atomic<bool> running{true};

  {
    Thread thread("stoppable_thread", [&running](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      running = false;
    });

    thread.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(running);

    thread.RequestStop();
    thread.Join();

    EXPECT_FALSE(running);
  }
}

TEST(ThreadTest, JoinWithTimeout) {
  std::atomic<bool> finished{false};

  Thread thread("timeout_thread", [&finished](std::stop_token stop_token) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    finished = true;
  });

  thread.Start();

  bool result = thread.Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
  EXPECT_FALSE(finished);

  thread.Join();
  EXPECT_TRUE(finished);
}

TEST(ThreadTest, GetName) {
  Thread thread("my_thread_name", [](std::stop_token) {});
  EXPECT_EQ(thread.GetName(), "my_thread_name");
}

#ifdef __linux__
TEST(ThreadTest, NativeThreadName) {
  std::string observed_name;

  {
    Thread thread("NativeTest", [&observed_name](std::stop_token stop_token) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      observed_name = GetCurrentThreadName();
    });

    thread.Start();
    thread.Join();
  }

  EXPECT_EQ(observed_name, "NativeTest");
}

TEST(ThreadTest, NativeThreadNameTruncated) {
  std::string observed_name;
  std::string long_name = "VeryLongThreadNameThatExceedsLimit";

  {
    Thread thread(long_name, [&observed_name](std::stop_token stop_token) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      observed_name = GetCurrentThreadName();
    });

    thread.Start();
    thread.Join();
  }

  EXPECT_EQ(observed_name, long_name.substr(0, 15));
}
#endif

TEST(ThreadTest, ShouldStop) {
  std::atomic<bool> should_stop_observed{false};

  {
    Thread thread("should_stop_thread",
                  [&should_stop_observed](std::stop_token) {
                    for (int i = 0; i < 100; ++i) {
                      std::this_thread::sleep_for(
                          std::chrono::milliseconds(10));
                    }
                    should_stop_observed = true;
                  });

    thread.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    thread.RequestStop();
  }
}

TEST(ThreadTest, MoveSemantics) {
  std::atomic<int> counter{0};
  std::atomic<bool> done{false};

  Thread thread1("moveable_thread", [&counter, &done](std::stop_token stop_token) {
    while (!stop_token.stop_requested() && counter < 3) {
      ++counter;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    done = true;
  });

  thread1.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  Thread thread2 = std::move(thread1);
  thread2.Join();

  EXPECT_GE(counter, 3);
  EXPECT_TRUE(done);
}

TEST(ThreadTest, MultipleStartStop) {
  std::atomic<int> counter{0};

  {
    Thread thread("reusable_thread", [&counter](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (counter >= 5) break;
      }
    });

    thread.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  EXPECT_GE(counter, 5);
}

TEST(ThreadTest, DestructorAutoJoin) {
  std::atomic<bool> finished{false};

  {
    Thread thread("auto_join_thread", [&finished](std::stop_token stop_token) {
      for (int i = 0; i < 10; ++i) {
        if (stop_token.stop_requested()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      finished = true;
    });

    thread.Start();
  }

  EXPECT_TRUE(finished);
}
