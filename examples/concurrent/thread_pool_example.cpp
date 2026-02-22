// Example: Thread Pool Task Submission
// Demonstrates fixed-size thread pool with future-based task results

#include <chrono>
#include <iostream>
#include <vector>

#include "cytoskeleton/concurrent/thread_pool.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic task submission
  std::cout << "=== Basic Task Submission ===" << std::endl;
  {
    ThreadPool pool(4);

    auto future = pool.Submit([]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      return 42;
    });

    std::cout << "Task result: " << future.get() << std::endl;
  }

  // Example 2: Multiple tasks with parameters
  std::cout << "\n=== Tasks with Parameters ===" << std::endl;
  {
    ThreadPool pool(4);
    std::vector<std::future<int>> futures;

    for (int i = 1; i <= 5; ++i) {
      futures.push_back(pool.Submit([](int n) {
        int sum = 0;
        for (int j = 1; j <= n; ++j) {
          sum += j;
        }
        return sum;
      }, i));
    }

    for (size_t i = 0; i < futures.size(); ++i) {
      std::cout << "Sum 1 to " << (i + 1) << " = " << futures[i].get() << std::endl;
    }
  }

  // Example 3: Void tasks (no return value)
  std::cout << "\n=== Void Tasks ===" << std::endl;
  {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 10; ++i) {
      futures.push_back(pool.Submit([&counter, i]() {
        std::cout << "Task " << i << " executing" << std::endl;
        ++counter;
      }));
    }

    for (auto& f : futures) {
      f.get();
    }

    std::cout << "Total executed: " << counter << std::endl;
  }

  // Example 4: Parallel computation
  std::cout << "\n=== Parallel Computation ===" << std::endl;
  {
    ThreadPool pool(4);
    const int num_tasks = 8;
    const int work_per_task = 1000000;

    std::vector<std::future<long long>> futures;

    auto start = std::chrono::steady_clock::now();

    for (int t = 0; t < num_tasks; ++t) {
      futures.push_back(pool.Submit([t, work_per_task]() -> long long {
        long long sum = 0;
        int start = t * work_per_task;
        int end = start + work_per_task;
        for (int i = start; i < end; ++i) {
          sum += i;
        }
        return sum;
      }));
    }

    long long total = 0;
    for (auto& f : futures) {
      total += f.get();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Total sum: " << total << std::endl;
    std::cout << "Time: " << duration.count() << "ms" << std::endl;
  }

  // Example 5: Exception handling
  std::cout << "\n=== Exception Handling ===" << std::endl;
  {
    ThreadPool pool(2);

    auto future = pool.Submit([]() -> int {
      throw std::runtime_error("Task failed!");
      return 0;
    });

    try {
      future.get();
    } catch (const std::runtime_error& e) {
      std::cout << "Caught exception: " << e.what() << std::endl;
    }
  }

  // Example 6: Different pool sizes
  std::cout << "\n=== Different Pool Sizes ===" << std::endl;
  {
    for (size_t pool_size : {1, 2, 4}) {
      ThreadPool pool(pool_size);
      std::atomic<int> completed{0};

      auto start = std::chrono::steady_clock::now();

      std::vector<std::future<void>> futures;
      for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.Submit([&completed]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          ++completed;
        }));
      }

      for (auto& f : futures) {
        f.get();
      }

      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      std::cout << "Pool size " << pool_size << ": " << duration.count() << "ms" << std::endl;
    }
  }

  return 0;
}
