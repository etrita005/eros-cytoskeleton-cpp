#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/thread_pool.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(ThreadPoolTest, BasicSubmit) {
  ThreadPool pool(4);

  std::future<int> result = pool.Submit([]() { return 42; });

  EXPECT_EQ(result.get(), 42);
}

TEST(ThreadPoolTest, MultipleTasks) {
  ThreadPool pool(4);
  std::vector<std::future<int>> futures;

  for (int i = 0; i < 10; ++i) {
    futures.push_back(pool.Submit([i]() { return i * i; }));
  }

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(futures[i].get(), i * i);
  }
}

TEST(ThreadPoolTest, ConcurrentExecution) {
  ThreadPool pool(4);
  std::atomic<int> counter{0};

  std::vector<std::future<void>> futures;
  for (int i = 0; i < 100; ++i) {
    futures.push_back(pool.Submit([&counter]() { ++counter; }));
  }

  for (auto& f : futures) {
    f.get();
  }

  EXPECT_EQ(counter, 100);
}

TEST(ThreadPoolTest, SubmitWithParameters) {
  ThreadPool pool(4);

  auto future = pool.Submit([](int a, int b) { return a + b; }, 10, 20);

  EXPECT_EQ(future.get(), 30);
}

TEST(ThreadPoolTest, VoidReturnType) {
  ThreadPool pool(4);
  std::atomic<int> counter{0};

  std::vector<std::future<void>> futures;
  for (int i = 0; i < 10; ++i) {
    futures.push_back(pool.Submit([&counter]() { ++counter; }));
  }

  for (auto& f : futures) {
    f.get();
  }

  EXPECT_EQ(counter, 10);
}

TEST(ThreadPoolTest, ExceptionHandling) {
  ThreadPool pool(4);

  auto future = pool.Submit([]() -> int {
    throw std::runtime_error("Test exception");
    return 42;
  });

  EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(ThreadPoolTest, Shutdown) {
  ThreadPool pool(4);

  std::atomic<int> counter{0};

  std::vector<std::future<void>> futures;
  for (int i = 0; i < 10; ++i) {
    futures.push_back(pool.Submit([&counter]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      ++counter;
    }));
  }

  pool.Shutdown();

  EXPECT_EQ(counter, 10);
}

TEST(ThreadPoolTest, DestructorAutoShutdown) {
  std::atomic<int> counter{0};

  {
    ThreadPool pool(4);

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
      futures.push_back(pool.Submit([&counter]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++counter;
      }));
    }
  }

  EXPECT_EQ(counter, 10);
}

TEST(ThreadPoolTest, DifferentPoolSizes) {
  for (size_t pool_size : {1, 2, 4, 8}) {
    ThreadPool pool(pool_size);

    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 20; ++i) {
      futures.push_back(pool.Submit([&counter]() { ++counter; }));
    }

    for (auto& f : futures) {
      f.get();
    }

    EXPECT_EQ(counter, 20);
  }
}

TEST(ThreadPoolTest, TaskOrdering) {
  ThreadPool pool(1);

  std::vector<int> order;
  std::mutex mutex;

  std::vector<std::future<void>> futures;
  for (int i = 0; i < 10; ++i) {
    futures.push_back(pool.Submit([&order, &mutex, i]() {
      std::lock_guard<std::mutex> lock(mutex);
      order.push_back(i);
    }));
  }

  for (auto& f : futures) {
    f.get();
  }

  EXPECT_EQ(order.size(), 10);
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(order[i], i);
  }
}

TEST(ThreadPoolTest, SubmitAfterShutdown) {
  ThreadPool pool(4);
  pool.Shutdown();

  EXPECT_THROW(pool.Submit([]() { return 42; }), std::runtime_error);
}

TEST(ThreadPoolTest, ComplexReturnTypes) {
  ThreadPool pool(4);

  auto future = pool.Submit([]() -> std::vector<int> {
    return {1, 2, 3, 4, 5};
  });

  std::vector<int> result = future.get();
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[4], 5);
}

TEST(ThreadPoolTest, ReferenceParameters) {
  ThreadPool pool(4);

  int value = 10;
  auto future = pool.Submit([](int& ref) { ref *= 2; }, std::ref(value));

  future.get();
  EXPECT_EQ(value, 20);
}

TEST(ThreadPoolTest, StressTest) {
  ThreadPool pool(8);
  std::atomic<int> counter{0};
  const int kNumTasks = 1000;

  std::vector<std::future<void>> futures;
  for (int i = 0; i < kNumTasks; ++i) {
    futures.push_back(pool.Submit([&counter]() {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      ++counter;
    }));
  }

  for (auto& f : futures) {
    f.get();
  }

  EXPECT_EQ(counter, kNumTasks);
}
