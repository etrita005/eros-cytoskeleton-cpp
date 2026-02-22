// Example: Thread-Safe Ordered Map
// Demonstrates concurrent map operations with ordered keys

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/map.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic operations
  std::cout << "=== Basic Operations ===" << std::endl;
  {
    Map<int, std::string> map;

    // Insert key-value pairs
    map.Insert(1, "one");
    map.Insert(2, "two");
    map.Insert(3, "three");

    std::cout << "Size: " << map.Size() << std::endl;

    // Retrieve values
    std::string value;
    if (map.TryGet(2, value)) {
      std::cout << "Key 2 maps to: " << value << std::endl;
    }

    // Check existence
    std::cout << "Contains key 3: " << (map.Contains(3) ? "yes" : "no") << std::endl;
    std::cout << "Contains key 5: " << (map.Contains(5) ? "yes" : "no") << std::endl;
  }

  // Example 2: Remove and retrieve
  std::cout << "\n=== Remove Operations ===" << std::endl;
  {
    Map<int, std::string> map;
    map.Insert(1, "one");
    map.Insert(2, "two");

    std::string removed;
    if (map.TryRemove(1, removed)) {
      std::cout << "Removed: " << removed << std::endl;
    }
    std::cout << "Size after remove: " << map.Size() << std::endl;

    // Try to remove non-existent key
    if (!map.TryRemove(99, removed)) {
      std::cout << "Key 99 not found" << std::endl;
    }
  }

  // Example 3: Iteration
  std::cout << "\n=== Iteration ===" << std::endl;
  {
    Map<int, std::string> map;
    map.Insert(3, "three");
    map.Insert(1, "one");
    map.Insert(2, "two");

    std::cout << "Ordered iteration: " << std::endl;
    map.ForEach([](const int& key, const std::string& val) {
      std::cout << "  " << key << " -> " << val << std::endl;
      return true;
    });
  }

  // Example 4: Keys and Values
  std::cout << "\n=== Keys and Values ===" << std::endl;
  {
    Map<int, std::string> map;
    map.Insert(10, "ten");
    map.Insert(20, "twenty");
    map.Insert(30, "thirty");

    auto keys = map.Keys();
    std::cout << "Keys: ";
    for (const auto& k : keys) {
      std::cout << k << " ";
    }
    std::cout << std::endl;

    auto values = map.Values();
    std::cout << "Values: ";
    for (const auto& v : values) {
      std::cout << v << " ";
    }
    std::cout << std::endl;
  }

  // Example 5: Concurrent insertions
  std::cout << "\n=== Concurrent Insertions ===" << std::endl;
  {
    Map<int, int> map;
    const int num_threads = 4;
    const int items_per_thread = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&map, i, items_per_thread]() {
        for (int j = 0; j < items_per_thread; ++j) {
          int key = i * items_per_thread + j;
          map.Insert(key, key * 2);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Total entries: " << map.Size() << std::endl;
  }

  return 0;
}
