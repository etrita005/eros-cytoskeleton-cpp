#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/stack.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(StackTest, PushAndSize) {
  Stack<int> stack;
  EXPECT_EQ(stack.Size(), 0);
  EXPECT_TRUE(stack.Empty());

  stack.Push(1);
  EXPECT_EQ(stack.Size(), 1);
  EXPECT_FALSE(stack.Empty());

  stack.Push(2);
  EXPECT_EQ(stack.Size(), 2);
}

TEST(StackTest, TryPopBasic) {
  Stack<int> stack;
  stack.Push(1);
  stack.Push(2);

  int value;
  EXPECT_TRUE(stack.TryPop(value));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(stack.Size(), 1);

  EXPECT_TRUE(stack.TryPop(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(stack.Empty());

  EXPECT_FALSE(stack.TryPop(value));
}

TEST(StackTest, TryPop) {
  Stack<int> stack;
  stack.Push(1);
  stack.Push(2);

  int value;
  EXPECT_TRUE(stack.TryPop(value));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(stack.Size(), 1);

  EXPECT_TRUE(stack.TryPop(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(stack.Empty());

  EXPECT_FALSE(stack.TryPop(value));
}

TEST(StackTest, TryGet) {
  Stack<int> stack;
  stack.Push(1);
  stack.Push(2);

  int value;
  EXPECT_TRUE(stack.TryGet(value));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(stack.Size(), 2);
}

TEST(StackTest, Clear) {
  Stack<int> stack;
  stack.Push(1);
  stack.Push(2);

  stack.Clear();
  EXPECT_EQ(stack.Size(), 0);
  EXPECT_TRUE(stack.Empty());
}

TEST(StackTest, ToVector) {
  Stack<int> stack;
  stack.Push(1);
  stack.Push(2);
  stack.Push(3);

  std::vector<int> result = stack.ToVector();
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 3);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 1);
}

TEST(StackTest, Filter) {
  Stack<int> stack;
  stack.Push(1);
  stack.Push(2);
  stack.Push(3);
  stack.Push(4);

  std::vector<int> result = stack.Filter([](const int& value) {
    return value % 2 == 0;
  });

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 4);
  EXPECT_EQ(result[1], 2);
}

TEST(StackTest, LIFOOrder) {
  Stack<int> stack;
  for (int i = 0; i < 100; ++i) {
    stack.Push(i);
  }

  for (int i = 99; i >= 0; --i) {
    int value;
    EXPECT_TRUE(stack.TryPop(value));
    EXPECT_EQ(value, i);
  }
}

TEST(StackTest, ConcurrentAccess) {
  Stack<int> stack;
  const int kNumThreads = 10;
  const int kElementsPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&stack, i]() {
      for (int j = 0; j < kElementsPerThread; ++j) {
        stack.Push(i * kElementsPerThread + j);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(stack.Size(), kNumThreads * kElementsPerThread);
}

TEST(StackTest, ProducerConsumer) {
  Stack<int> stack;
  std::atomic<int> sum{0};
  const int kNumElements = 1000;

  std::thread producer([&]() {
    for (int i = 1; i <= kNumElements; ++i) {
      stack.Push(i);
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < kNumElements; ++i) {
      int value;
      while (!stack.TryPop(value)) {
        std::this_thread::yield();
      }
      sum += value;
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(sum, kNumElements * (kNumElements + 1) / 2);
}

TEST(StackTest, MoveOnlyType) {
  Stack<std::unique_ptr<int>> stack;
  stack.Push(std::make_unique<int>(1));
  stack.Push(std::make_unique<int>(2));

  EXPECT_EQ(stack.Size(), 2);

  std::unique_ptr<int> value;
  EXPECT_TRUE(stack.TryPop(value));
  EXPECT_EQ(*value, 2);
}

TEST(StackTest, BlockingPop) {
  Stack<int> stack;
  std::atomic<bool> popped{false};
  int result = 0;

  std::thread consumer([&]() {
    stack.Pop(result);
    popped = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(popped);

  stack.Push(42);
  consumer.join();

  EXPECT_TRUE(popped);
  EXPECT_EQ(result, 42);
}

TEST(StackTest, BlockingProducerConsumer) {
  Stack<int> stack;
  std::atomic<int> sum{0};
  const int kNumElements = 1000;

  std::thread producer([&]() {
    for (int i = 1; i <= kNumElements; ++i) {
      stack.Push(i);
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < kNumElements; ++i) {
      int value;
      stack.Pop(value);
      sum += value;
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(sum, kNumElements * (kNumElements + 1) / 2);
}
