#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/map.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(MapTest, InsertAndSize) {
  Map<int, std::string> map;
  EXPECT_EQ(map.Size(), 0);
  EXPECT_TRUE(map.Empty());

  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_EQ(map.Size(), 1);
  EXPECT_FALSE(map.Empty());

  EXPECT_TRUE(map.Insert(2, "two"));
  EXPECT_EQ(map.Size(), 2);
}

TEST(MapTest, DuplicateInsert) {
  Map<int, std::string> map;
  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_FALSE(map.Insert(1, "another"));
  EXPECT_EQ(map.Size(), 1);
}

TEST(MapTest, TryGet) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::string value;
  EXPECT_TRUE(map.TryGet(1, value));
  EXPECT_EQ(value, "one");

  EXPECT_TRUE(map.TryGet(2, value));
  EXPECT_EQ(value, "two");

  EXPECT_FALSE(map.TryGet(3, value));
}

TEST(MapTest, TryRemove) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::string value;
  EXPECT_TRUE(map.TryRemove(1, value));
  EXPECT_EQ(value, "one");
  EXPECT_EQ(map.Size(), 1);

  EXPECT_FALSE(map.TryRemove(1, value));
  EXPECT_FALSE(map.TryRemove(3, value));
}

TEST(MapTest, Contains) {
  Map<int, std::string> map;
  map.Insert(1, "one");

  EXPECT_TRUE(map.Contains(1));
  EXPECT_FALSE(map.Contains(2));
}

TEST(MapTest, Clear) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  map.Clear();
  EXPECT_EQ(map.Size(), 0);
  EXPECT_TRUE(map.Empty());
}

TEST(MapTest, ForEach) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");
  map.Insert(3, "three");

  int sum = 0;
  map.ForEach([&sum](const int& key, const std::string& value) {
    (void)value;
    sum += key;
    return true;
  });

  EXPECT_EQ(sum, 6);
}

TEST(MapTest, ForEachEarlyTermination) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");
  map.Insert(3, "three");

  int count = 0;
  map.ForEach([&count](const int& key, const std::string& value) {
    (void)key;
    (void)value;
    ++count;
    return count < 2;
  });

  EXPECT_EQ(count, 2);
}

TEST(MapTest, ToVector) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::vector<std::pair<int, std::string>> result = map.ToVector();
  EXPECT_EQ(result.size(), 2);
}

TEST(MapTest, Keys) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");
  map.Insert(3, "three");

  std::vector<int> keys = map.Keys();
  EXPECT_EQ(keys.size(), 3);
  EXPECT_EQ(keys[0], 1);
  EXPECT_EQ(keys[1], 2);
  EXPECT_EQ(keys[2], 3);
}

TEST(MapTest, Values) {
  Map<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::vector<std::string> values = map.Values();
  EXPECT_EQ(values.size(), 2);
}

TEST(MapTest, ConcurrentAccess) {
  Map<int, int> map;
  const int kNumThreads = 10;
  const int kElementsPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&map, i]() {
      for (int j = 0; j < kElementsPerThread; ++j) {
        int key = i * kElementsPerThread + j;
        map.Insert(key, key * 2);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(map.Size(), kNumThreads * kElementsPerThread);
}

TEST(MapTest, MoveOnlyType) {
  Map<int, std::unique_ptr<std::string>> map;
  map.Insert(1, std::make_unique<std::string>("one"));
  map.Insert(2, std::make_unique<std::string>("two"));

  EXPECT_EQ(map.Size(), 2);

  std::unique_ptr<std::string> value;
  EXPECT_TRUE(map.TryRemove(1, value));
  EXPECT_EQ(*value, "one");
}
