// Example: Thread-Safe Hash Map
// Demonstrates concurrent unordered map operations

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/hash_map.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic operations with string keys
  std::cout << "=== String Key Operations ===" << std::endl;
  {
    HashMap<std::string, int> scores;

    scores.Insert("Alice", 95);
    scores.Insert("Bob", 87);
    scores.Insert("Charlie", 92);

    int score;
    if (scores.TryGet("Alice", score)) {
      std::cout << "Alice's score: " << score << std::endl;
    }

    std::cout << "Total students: " << scores.Size() << std::endl;
  }

  // Example 2: Duplicate handling
  std::cout << "\n=== Duplicate Handling ===" << std::endl;
  {
    HashMap<int, std::string> map;

    bool inserted1 = map.Insert(1, "first");
    bool inserted2 = map.Insert(1, "second");  // Should fail

    std::cout << "First insert: " << (inserted1 ? "success" : "failed") << std::endl;
    std::cout << "Second insert: " << (inserted2 ? "success" : "failed") << std::endl;

    std::string val;
    map.TryGet(1, val);
    std::cout << "Value at key 1: " << val << std::endl;
  }

  // Example 3: Update pattern
  std::cout << "\n=== Update Pattern ===" << std::endl;
  {
    HashMap<std::string, int> counters;

    // Increment counter pattern
    auto increment = [&counters](const std::string& key) {
      int current;
      if (counters.TryGet(key, current)) {
        counters.TryRemove(key, current);
      }
      counters.Insert(key, current + 1);
    };

    increment("requests");
    increment("requests");
    increment("requests");

    int count;
    if (counters.TryGet("requests", count)) {
      std::cout << "Request count: " << count << std::endl;
    }
  }

  // Example 4: Batch operations
  std::cout << "\n=== Batch Operations ===" << std::endl;
  {
    HashMap<int, std::string> map;

    // Insert batch
    for (int i = 0; i < 10; ++i) {
      map.Insert(i, "value_" + std::to_string(i));
    }

    // Convert to vector
    auto entries = map.ToVector();
    std::cout << "Entries count: " << entries.size() << std::endl;

    // Get all keys
    auto keys = map.Keys();
    std::cout << "Keys count: " << keys.size() << std::endl;
  }

  // Example 5: Concurrent access with high contention
  std::cout << "\n=== High Contention Test ===" << std::endl;
  {
    HashMap<int, int> map;
    const int num_threads = 8;
    const int operations = 1000;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&map, t, operations]() {
        for (int i = 0; i < operations; ++i) {
          int key = i % 100;  // High contention on same keys
          int value;

          // Try to get and increment
          if (map.TryGet(key, value)) {
            map.TryRemove(key, value);
            map.Insert(key, value + 1);
          } else {
            map.Insert(key, 1);
          }
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Final unique keys: " << map.Size() << std::endl;

    // Sum all values
    int total = 0;
    map.ForEach([&total](const int& key, const int& val) {
      total += val;
      return true;
    });
    std::cout << "Total operations recorded: " << total << std::endl;
  }

  return 0;
}
