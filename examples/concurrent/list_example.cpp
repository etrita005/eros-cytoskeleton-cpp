// Example: Thread-Safe Doubly Linked List
// Demonstrates list operations with index-based access

#include <iostream>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/list.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic operations
  std::cout << "=== Basic Operations ===" << std::endl;
  {
    List<int> list;

    list.PushBack(20);
    list.PushFront(10);
    list.PushBack(30);

    std::cout << "Size: " << list.Size() << std::endl;

    int val;
    for (size_t i = 0; i < list.Size(); ++i) {
      if (list.TryGet(i, val)) {
        std::cout << "Element " << i << ": " << val << std::endl;
      }
    }
  }

  // Example 2: Pop operations
  std::cout << "\n=== Pop Operations ===" << std::endl;
  {
    List<int> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);

    int val;
    if (list.PopFront(val)) {
      std::cout << "Popped front: " << val << std::endl;
    }
    if (list.PopBack(val)) {
      std::cout << "Popped back: " << val << std::endl;
    }
    std::cout << "Remaining: " << list.Size() << std::endl;
  }

  // Example 3: Slice operation
  std::cout << "\n=== Slice Operation ===" << std::endl;
  {
    List<int> list;
    for (int i = 0; i < 10; ++i) {
      list.PushBack(i * 10);
    }

    auto slice = list.Slice(3, 7);
    std::cout << "Slice [3, 7): ";
    for (int v : slice) {
      std::cout << v << " ";
    }
    std::cout << std::endl;
  }

  // Example 4: Filter operation
  std::cout << "\n=== Filter Operation ===" << std::endl;
  {
    List<int> list;
    for (int i = 1; i <= 20; ++i) {
      list.PushBack(i);
    }

    auto primes = list.Filter([](const int& n) {
      if (n < 2) return false;
      for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
      }
      return true;
    });

    std::cout << "Prime numbers: ";
    for (int p : primes) {
      std::cout << p << " ";
    }
    std::cout << std::endl;
  }

  // Example 5: Concurrent modifications
  std::cout << "\n=== Concurrent Modifications ===" << std::endl;
  {
    List<int> list;
    const int num_threads = 4;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&list, i]() {
        for (int j = 0; j < 25; ++j) {
          if (j % 2 == 0) {
            list.PushBack(i * 100 + j);
          } else {
            list.PushFront(i * 100 + j);
          }
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Total elements: " << list.Size() << std::endl;
  }

  return 0;
}
