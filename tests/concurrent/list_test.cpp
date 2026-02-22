#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/list.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(ListTest, PushBackAndSize) {
  List<int> list;
  EXPECT_EQ(list.Size(), 0);
  EXPECT_TRUE(list.Empty());

  list.PushBack(1);
  EXPECT_EQ(list.Size(), 1);
  EXPECT_FALSE(list.Empty());

  list.PushBack(2);
  EXPECT_EQ(list.Size(), 2);
}

TEST(ListTest, PushFront) {
  List<int> list;
  list.PushBack(2);
  list.PushBack(3);
  list.PushFront(1);

  EXPECT_EQ(list.Size(), 3);
  int value;
  EXPECT_TRUE(list.TryGet(0, value));
  EXPECT_EQ(value, 1);
}

TEST(ListTest, PopBack) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);

  int value;
  EXPECT_TRUE(list.PopBack(value));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(list.Size(), 1);

  EXPECT_TRUE(list.PopBack(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(list.Empty());

  EXPECT_FALSE(list.PopBack(value));
}

TEST(ListTest, PopFront) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);

  int value;
  EXPECT_TRUE(list.PopFront(value));
  EXPECT_EQ(value, 1);
  EXPECT_EQ(list.Size(), 1);

  EXPECT_TRUE(list.PopFront(value));
  EXPECT_EQ(value, 2);
  EXPECT_TRUE(list.Empty());
}

TEST(ListTest, TryGet) {
  List<int> list;
  list.PushBack(10);
  list.PushBack(20);

  int value;
  EXPECT_TRUE(list.TryGet(0, value));
  EXPECT_EQ(value, 10);

  EXPECT_TRUE(list.TryGet(1, value));
  EXPECT_EQ(value, 20);

  EXPECT_FALSE(list.TryGet(2, value));
}

TEST(ListTest, Clear) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);

  list.Clear();
  EXPECT_EQ(list.Size(), 0);
  EXPECT_TRUE(list.Empty());
}

TEST(ListTest, ForEach) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);
  list.PushBack(3);

  int sum = 0;
  list.ForEach([&sum](const int& value) {
    sum += value;
    return true;
  });

  EXPECT_EQ(sum, 6);
}

TEST(ListTest, ForEachEarlyTermination) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);
  list.PushBack(3);

  int count = 0;
  list.ForEach([&count](const int& value) {
    (void)value;
    ++count;
    return count < 2;
  });

  EXPECT_EQ(count, 2);
}

TEST(ListTest, Filter) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);
  list.PushBack(3);
  list.PushBack(4);

  std::vector<int> result = list.Filter([](const int& value) {
    return value % 2 == 0;
  });

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 2);
  EXPECT_EQ(result[1], 4);
}

TEST(ListTest, Slice) {
  List<int> list;
  list.PushBack(0);
  list.PushBack(1);
  list.PushBack(2);
  list.PushBack(3);
  list.PushBack(4);

  std::vector<int> result = list.Slice(1, 4);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST(ListTest, ToVector) {
  List<int> list;
  list.PushBack(1);
  list.PushBack(2);
  list.PushBack(3);

  std::vector<int> result = list.ToVector();
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST(ListTest, ConcurrentAccess) {
  List<int> list;
  const int kNumThreads = 10;
  const int kElementsPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&list, i]() {
      for (int j = 0; j < kElementsPerThread; ++j) {
        list.PushBack(i * kElementsPerThread + j);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(list.Size(), kNumThreads * kElementsPerThread);
}

TEST(ListTest, MoveOnlyType) {
  List<std::unique_ptr<int>> list;
  list.PushBack(std::make_unique<int>(1));
  list.PushBack(std::make_unique<int>(2));

  EXPECT_EQ(list.Size(), 2);

  std::unique_ptr<int> value;
  EXPECT_TRUE(list.PopBack(value));
  EXPECT_EQ(*value, 2);
}
