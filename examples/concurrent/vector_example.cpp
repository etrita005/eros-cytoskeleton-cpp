// Example: Thread-Safe Vector
// Demonstrates concurrent vector operations

#include <iostream>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/vector.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic operations
  std::cout << "=== Basic Operations ===" << std::endl;
  {
    Vector<int> vec;

    vec.PushBack(10);
    vec.PushBack(20);
    vec.PushFront(5);

    std::cout << "Size: " << vec.Size() << std::endl;
    std::cout << "Element at 0: " << vec[0] << std::endl;
    std::cout << "Element at 1: " << vec[1] << std::endl;

    int value;
    if (vec.TryGet(2, value)) {
      std::cout << "Element at 2: " << value << std::endl;
    }
  }

  // Example 2: Pop operations
  std::cout << "\n=== Pop Operations ===" << std::endl;
  {
    Vector<int> vec;
    vec.PushBack(1);
    vec.PushBack(2);
    vec.PushBack(3);

    int value;
    if (vec.PopBack(value)) {
      std::cout << "Popped from back: " << value << std::endl;
    }
    if (vec.PopFront(value)) {
      std::cout << "Popped from front: " << value << std::endl;
    }
    std::cout << "Remaining size: " << vec.Size() << std::endl;
  }

  // Example 3: ForEach iteration
  std::cout << "\n=== ForEach Iteration ===" << std::endl;
  {
    Vector<int> vec;
    for (int i = 1; i <= 5; ++i) {
      vec.PushBack(i);
    }

    std::cout << "All elements: ";
    vec.ForEach([](const int& val) {
      std::cout << val << " ";
      return true;  // Continue iteration
    });
    std::cout << std::endl;
  }

  // Example 4: Filter and Slice
  std::cout << "\n=== Filter and Slice ===" << std::endl;
  {
    Vector<int> vec;
    for (int i = 1; i <= 10; ++i) {
      vec.PushBack(i);
    }

    // Filter even numbers
    auto evens = vec.Filter([](const int& val) { return val % 2 == 0; });
    std::cout << "Even numbers: ";
    for (int v : evens) {
      std::cout << v << " ";
    }
    std::cout << std::endl;

    // Get slice [3, 6)
    auto slice = vec.Slice(3, 6);
    std::cout << "Slice [3,6): ";
    for (int v : slice) {
      std::cout << v << " ";
    }
    std::cout << std::endl;
  }

  // Example 5: Concurrent access
  std::cout << "\n=== Concurrent Access ===" << std::endl;
  {
    Vector<int> vec;
    const int num_threads = 4;
    const int items_per_thread = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&vec, i, items_per_thread]() {
        for (int j = 0; j < items_per_thread; ++j) {
          vec.PushBack(i * items_per_thread + j);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Total elements: " << vec.Size() << std::endl;
    std::cout << "Expected: " << num_threads * items_per_thread << std::endl;
  }

  return 0;
}
