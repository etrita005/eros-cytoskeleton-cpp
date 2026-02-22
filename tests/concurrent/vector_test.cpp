#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/vector.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(VectorTest, PushBackAndSize) {
  Vector<int> vec;
  EXPECT_EQ(vec.Size(), 0);
  EXPECT_TRUE(vec.Empty());

  vec.PushBack(1);
  EXPECT_EQ(vec.Size(), 1);
  EXPECT_FALSE(vec.Empty());

  vec.PushBack(2);
  EXPECT_EQ(vec.Size(), 2);
}

TEST(VectorTest, PushFront) {
  Vector<int> vec;
  vec.PushBack(2);
  vec.PushBack(3);
  vec.PushFront(1);

  EXPECT_EQ(vec.Size(), 3);
  int value;
  EXPECT_TRUE(vec.TryGet(0, value));
  EXPECT_EQ(value, 1);
}

TEST(VectorTest, PopBack) {
  Vector<int> vec;
  vec.PushBack(1);
  vec.PushBack(2);

  int value;
  EXPECT_TRUE(vec.PopBack(value));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(vec.Size(), 1);

  EXPECT_TRUE(vec.PopBack(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(vec.Empty());

  EXPECT_FALSE(vec.PopBack(value));
}

TEST(VectorTest, PopFront) {
  Vector<int> vec;
  vec.PushBack(1);
  vec.PushBack(2);

  int value;
  EXPECT_TRUE(vec.PopFront(value));
  EXPECT_EQ(value, 1);
  EXPECT_EQ(vec.Size(), 1);

  EXPECT_TRUE(vec.PopFront(value));
  EXPECT_EQ(value, 2);
  EXPECT_TRUE(vec.Empty());
}

TEST(VectorTest, TryGet) {
  Vector<int> vec;
  vec.PushBack(10);
  vec.PushBack(20);

  int value;
  EXPECT_TRUE(vec.TryGet(0, value));
  EXPECT_EQ(value, 10);

  EXPECT_TRUE(vec.TryGet(1, value));
  EXPECT_EQ(value, 20);

  EXPECT_FALSE(vec.TryGet(2, value));
}

TEST(VectorTest, SubscriptOperator) {
  Vector<int> vec;
  vec.PushBack(10);
  vec.PushBack(20);

  EXPECT_EQ(vec[0], 10);
  EXPECT_EQ(vec[1], 20);
}

TEST(VectorTest, Clear) {
  Vector<int> vec;
  vec.PushBack(1);
  vec.PushBack(2);

  vec.Clear();
  EXPECT_EQ(vec.Size(), 0);
  EXPECT_TRUE(vec.Empty());
}

TEST(VectorTest, ForEach) {
  Vector<int> vec;
  vec.PushBack(1);
  vec.PushBack(2);
  vec.PushBack(3);

  int sum = 0;
  vec.ForEach([&sum](const int& value) {
    sum += value;
    return true;
  });

  EXPECT_EQ(sum, 6);
}

TEST(VectorTest, ForEachEarlyTermination) {
  Vector<int> vec;
  vec.PushBack(1);
  vec.PushBack(2);
  vec.PushBack(3);

  int count = 0;
  vec.ForEach([&count](const int& value) {
    (void)value;
    ++count;
    return count < 2;
  });

  EXPECT_EQ(count, 2);
}

TEST(VectorTest, Filter) {
  Vector<int> vec;
  vec.PushBack(1);
  vec.PushBack(2);
  vec.PushBack(3);
  vec.PushBack(4);

  std::vector<int> result = vec.Filter([](const int& value) {
    return value % 2 == 0;
  });

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 4);
}

TEST(VectorTest, Slice) {
  Vector<int> vec;
  vec.PushBack(0);
  vec.PushBack(1);
  vec.PushBack(2);
  vec.PushBack(3);
  vec.PushBack(4);

  std::vector<int> result = vec.Slice(1, 4);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST(VectorTest, SliceEdgeCases) {
  Vector<int> vec;
  vec.PushBack(0);
  vec.PushBack(1);

  std::vector<int> result1 = vec.Slice(5, 10);
  EXPECT_TRUE(result1.empty());

  std::vector<int> result2 = vec.Slice(1, 1);
  EXPECT_TRUE(result2.empty());

  std::vector<int> result3 = vec.Slice(0, 10);
  EXPECT_EQ(result3.size(), 2);
}

TEST(VectorTest, ConcurrentAccess) {
  Vector<int> vec;
  const int kNumThreads = 10;
  const int kElementsPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&vec, i]() {
      for (int j = 0; j < kElementsPerThread; ++j) {
        vec.PushBack(i * kElementsPerThread + j);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(vec.Size(), kNumThreads * kElementsPerThread);
}

TEST(VectorTest, MoveOnlyType) {
  Vector<std::unique_ptr<int>> vec;
  vec.PushBack(std::make_unique<int>(1));
  vec.PushBack(std::make_unique<int>(2));

  EXPECT_EQ(vec.Size(), 2);

  std::unique_ptr<int> value;
  EXPECT_TRUE(vec.PopBack(value));
  EXPECT_EQ(*value, 2);
}
