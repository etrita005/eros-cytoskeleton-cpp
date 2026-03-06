// Example: Thread Lifecycle Management
// Demonstrates Thread wrapper with C++20 stop tokens

#include <chrono>
#include <iostream>
#include <thread>

#include "cytoskeleton/concurrent/thread.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Lambda-based thread
  std::cout << "=== Lambda-Based Thread ===" << std::endl;
  {
    std::atomic<int> counter{0};

    Thread worker("counter", [&counter](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Count: " << counter << std::endl;
        if (counter >= 5) break;
      }
    });

    worker.Start();
    worker.Join();
    std::cout << "Final count: " << counter << std::endl;
  }

  // Example 2: Function-based thread with state
  std::cout << "\n=== Function-Based Thread with State ===" << std::endl;
  {
    std::atomic<int> processed{0};

    Thread worker("worker", [&processed](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        std::cout << "Processing item " << ++processed << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (processed >= 3) break;
      }
    });

    worker.Start();
    worker.Join();
    std::cout << "Total processed: " << processed << std::endl;
  }

  // Example 3: Graceful shutdown with stop token
  std::cout << "\n=== Graceful Shutdown ===" << std::endl;
  {
    std::atomic<bool> running{true};

    Thread service("service", [&running](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        std::cout << "Service running..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
      }
      running = false;
      std::cout << "Service stopped gracefully" << std::endl;
    });

    service.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    std::cout << "Requesting stop..." << std::endl;
    service.RequestStop();
    service.Join();

    std::cout << "Running state: " << (running ? "true" : "false") << std::endl;
  }

  // Example 4: Timeout join
  std::cout << "\n=== Timeout Join ===" << std::endl;
  {
    Thread slow_worker("slow", [](std::stop_token) {
      std::cout << "Slow worker started" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      std::cout << "Slow worker finished" << std::endl;
    });

    slow_worker.Start();

    std::cout << "Waiting with 300ms timeout..." << std::endl;
    bool joined = slow_worker.Join(std::chrono::milliseconds(300));
    std::cout << "Timeout join result: " << (joined ? "joined" : "timeout") << std::endl;

    if (!joined) {
      std::cout << "Waiting for actual completion..." << std::endl;
      slow_worker.Join();
    }
  }

  // Example 5: Thread name
  std::cout << "\n=== Thread Name ===" << std::endl;
  {
    Thread named("my_custom_thread", [](std::stop_token) {
      std::cout << "Thread executing" << std::endl;
    });

    std::cout << "Thread name: " << named.GetName() << std::endl;
    named.Start();
    named.Join();
  }

  // Example 6: Automatic cleanup on destruction
  std::cout << "\n=== Automatic Cleanup ===" << std::endl;
  {
    std::atomic<bool> finished{false};

    {
      Thread auto_thread("auto_cleanup", [&finished](std::stop_token stop_token) {
        for (int i = 0; i < 5; ++i) {
          if (stop_token.stop_requested()) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        finished = true;
        std::cout << "Thread finished" << std::endl;
      });

      auto_thread.Start();
      // Thread destructor will call RequestStop() and Join()
    }

    std::cout << "Thread auto-cleaned, finished: " << (finished ? "true" : "false") << std::endl;
  }

  return 0;
}
