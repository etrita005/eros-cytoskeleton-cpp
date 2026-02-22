// Example: Thread-Safe Queue (Producer-Consumer Pattern)
// Demonstrates FIFO queue with blocking and non-blocking operations

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/queue.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic FIFO operations
  std::cout << "=== Basic FIFO Operations ===" << std::endl;
  {
    Queue<int> queue;

    queue.Enqueue(10);
    queue.Enqueue(20);
    queue.Enqueue(30);

    std::cout << "Queue size: " << queue.Size() << std::endl;

    int value;
    while (queue.TryDequeue(value)) {
      std::cout << "Dequeued: " << value << std::endl;
    }
  }

  // Example 2: Peek without removing
  std::cout << "\n=== Peek Operation ===" << std::endl;
  {
    Queue<std::string> queue;
    queue.Enqueue("first");
    queue.Enqueue("second");

    std::string front;
    if (queue.TryGet(front)) {
      std::cout << "Front element: " << front << std::endl;
      std::cout << "Size after peek: " << queue.Size() << std::endl;
    }
  }

  // Example 3: Single producer, single consumer
  std::cout << "\n=== Single Producer/Consumer ===" << std::endl;
  {
    Queue<int> queue;
    const int num_items = 100;
    int sum = 0;

    std::thread producer([&]() {
      for (int i = 1; i <= num_items; ++i) {
        queue.Enqueue(i);
      }
    });

    std::thread consumer([&]() {
      for (int i = 0; i < num_items; ++i) {
        int value;
        queue.Dequeue(value);  // Blocking
        sum += value;
      }
    });

    producer.join();
    consumer.join();

    std::cout << "Sum of 1 to " << num_items << " = " << sum << std::endl;
    std::cout << "Expected: " << (num_items * (num_items + 1) / 2) << std::endl;
  }

  // Example 4: Multiple producers, single consumer
  std::cout << "\n=== Multiple Producers/Single Consumer ===" << std::endl;
  {
    Queue<int> queue;
    const int num_producers = 4;
    const int items_per_producer = 100;
    std::atomic<int> consumed{0};

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
      producers.emplace_back([&queue, i, items_per_producer]() {
        for (int j = 0; j < items_per_producer; ++j) {
          queue.Enqueue(i * items_per_producer + j);
        }
      });
    }

    std::thread consumer([&]() {
      int expected = num_producers * items_per_producer;
      while (consumed < expected) {
        int value;
        if (queue.TryDequeue(value)) {
          ++consumed;
        } else {
          std::this_thread::yield();
        }
      }
    });

    for (auto& t : producers) {
      t.join();
    }
    consumer.join();

    std::cout << "Total consumed: " << consumed << std::endl;
  }

  // Example 5: Work distribution
  std::cout << "\n=== Work Distribution ===" << std::endl;
  {
    Queue<std::function<void()>> work_queue;
    std::atomic<int> completed{0};

    // Add work items
    for (int i = 0; i < 10; ++i) {
      work_queue.Enqueue([&completed, i]() {
        std::cout << "Processing task " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++completed;
      });
    }

    // Process all work
    std::vector<std::thread> workers;
    for (int i = 0; i < 3; ++i) {
      workers.emplace_back([&work_queue, &completed]() {
        while (completed < 10) {
          std::function<void()> task;
          if (work_queue.TryDequeue(task)) {
            task();
          } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
        }
      });
    }

    for (auto& t : workers) {
      t.join();
    }

    std::cout << "Total completed: " << completed << std::endl;
  }

  return 0;
}
