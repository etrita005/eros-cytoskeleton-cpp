#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/queue.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(QueueTest, EnqueueAndSize) {
  Queue<int> queue;
  EXPECT_EQ(queue.Size(), 0);
  EXPECT_TRUE(queue.Empty());

  queue.Enqueue(1);
  EXPECT_EQ(queue.Size(), 1);
  EXPECT_FALSE(queue.Empty());

  queue.Enqueue(2);
  EXPECT_EQ(queue.Size(), 2);
}

TEST(QueueTest, TryDequeue) {
  Queue<int> queue;
  queue.Enqueue(1);
  queue.Enqueue(2);

  int value;
  EXPECT_TRUE(queue.TryDequeue(value));
  EXPECT_EQ(value, 1);
  EXPECT_EQ(queue.Size(), 1);

  EXPECT_TRUE(queue.TryDequeue(value));
  EXPECT_EQ(value, 2);
  EXPECT_TRUE(queue.Empty());

  EXPECT_FALSE(queue.TryDequeue(value));
}

TEST(QueueTest, TryGet) {
  Queue<int> queue;
  queue.Enqueue(1);
  queue.Enqueue(2);

  int value;
  EXPECT_TRUE(queue.TryGet(value));
  EXPECT_EQ(value, 1);
  EXPECT_EQ(queue.Size(), 2);
}

TEST(QueueTest, Clear) {
  Queue<int> queue;
  queue.Enqueue(1);
  queue.Enqueue(2);

  queue.Clear();
  EXPECT_EQ(queue.Size(), 0);
  EXPECT_TRUE(queue.Empty());
}

TEST(QueueTest, ToVector) {
  Queue<int> queue;
  queue.Enqueue(1);
  queue.Enqueue(2);
  queue.Enqueue(3);

  std::vector<int> result = queue.ToVector();
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST(QueueTest, Filter) {
  Queue<int> queue;
  queue.Enqueue(1);
  queue.Enqueue(2);
  queue.Enqueue(3);
  queue.Enqueue(4);

  std::vector<int> result = queue.Filter([](const int& value) {
    return value % 2 == 0;
  });

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 4);
}

TEST(QueueTest, FIFOOrder) {
  Queue<int> queue;
  for (int i = 0; i < 100; ++i) {
    queue.Enqueue(i);
  }

  for (int i = 0; i < 100; ++i) {
    int value;
    EXPECT_TRUE(queue.TryDequeue(value));
    EXPECT_EQ(value, i);
  }
}

TEST(QueueTest, ConcurrentProducers) {
  Queue<int> queue;
  const int kNumThreads = 10;
  const int kElementsPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&queue, i]() {
      for (int j = 0; j < kElementsPerThread; ++j) {
        queue.Enqueue(i * kElementsPerThread + j);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(queue.Size(), kNumThreads * kElementsPerThread);
}

TEST(QueueTest, ProducerConsumer) {
  Queue<int> queue;
  std::atomic<int> sum{0};
  const int kNumElements = 1000;

  std::thread producer([&]() {
    for (int i = 1; i <= kNumElements; ++i) {
      queue.Enqueue(i);
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < kNumElements; ++i) {
      int value;
      queue.Dequeue(value);
      sum += value;
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(sum, kNumElements * (kNumElements + 1) / 2);
}

TEST(QueueTest, MoveOnlyType) {
  Queue<std::unique_ptr<int>> queue;
  queue.Enqueue(std::make_unique<int>(1));
  queue.Enqueue(std::make_unique<int>(2));

  EXPECT_EQ(queue.Size(), 2);

  std::unique_ptr<int> value;
  EXPECT_TRUE(queue.TryDequeue(value));
  EXPECT_EQ(*value, 1);
}

TEST(QueueTest, BlockingDequeue) {
  Queue<int> queue;
  std::atomic<bool> dequeued{false};
  int result = 0;

  std::thread consumer([&]() {
    queue.Dequeue(result);
    dequeued = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(dequeued);

  queue.Enqueue(42);
  consumer.join();

  EXPECT_TRUE(dequeued);
  EXPECT_EQ(result, 42);
}
