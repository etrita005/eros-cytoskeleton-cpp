// Example: Mutex and Lock Guards Usage
// Demonstrates Mutex, ReadWriteMutex, and RAII lock guards

#include <iostream>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic Mutex usage
  std::cout << "=== Mutex Example ===" << std::endl;
  {
    Mutex mutex;
    int counter = 0;

    // Manual lock/unlock
    mutex.Lock();
    counter++;
    std::cout << "Counter: " << counter << std::endl;
    mutex.Unlock();

    // RAII lock guard (recommended)
    {
      MutexLock lock(mutex);
      counter++;
      std::cout << "Counter with RAII: " << counter << std::endl;
    }  // Automatically unlocked here
  }

  // Example 2: Recursive mutex (same thread can lock multiple times)
  std::cout << "\n=== Recursive Mutex Example ===" << std::endl;
  {
    Mutex mutex;
    mutex.Lock();
    mutex.Lock();  // OK - recursive
    std::cout << "Locked twice in same thread" << std::endl;
    mutex.Unlock();
    mutex.Unlock();
  }

  // Example 3: ReadWriteMutex for read-heavy workloads
  std::cout << "\n=== ReadWriteMutex Example ===" << std::endl;
  {
    ReadWriteMutex rw_mutex;
    int data = 42;

    // Multiple readers can hold read lock simultaneously
    {
      ReadLock lock(rw_mutex);
      std::cout << "Reading data: " << data << std::endl;
    }

    // Only one writer can hold write lock
    {
      WriteLock lock(rw_mutex);
      data = 100;
      std::cout << "Writing data: " << data << std::endl;
    }
  }

  // Example 4: Concurrent access protection
  std::cout << "\n=== Concurrent Access Example ===" << std::endl;
  {
    Mutex mutex;
    int shared_counter = 0;
    const int num_threads = 4;
    const int increments = 1000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&]() {
        for (int j = 0; j < increments; ++j) {
          MutexLock lock(mutex);
          ++shared_counter;
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Expected: " << num_threads * increments
              << ", Actual: " << shared_counter << std::endl;
  }

  return 0;
}
