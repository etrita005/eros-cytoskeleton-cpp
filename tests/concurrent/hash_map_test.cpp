#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/hash_map.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(HashMapTest, InsertAndSize) {
  HashMap<int, std::string> map;
  EXPECT_EQ(map.Size(), 0);
  EXPECT_TRUE(map.Empty());

  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_EQ(map.Size(), 1);
  EXPECT_FALSE(map.Empty());

  EXPECT_TRUE(map.Insert(2, "two"));
  EXPECT_EQ(map.Size(), 2);
}

TEST(HashMapTest, DuplicateInsert) {
  HashMap<int, std::string> map;
  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_FALSE(map.Insert(1, "another"));
  EXPECT_EQ(map.Size(), 1);
}

TEST(HashMapTest, TryGet) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::string value;
  EXPECT_TRUE(map.TryGet(1, value));
  EXPECT_EQ(value, "one");

  EXPECT_TRUE(map.TryGet(2, value));
  EXPECT_EQ(value, "two");

  EXPECT_FALSE(map.TryGet(3, value));
}

TEST(HashMapTest, TryRemove) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::string value;
  EXPECT_TRUE(map.TryRemove(1, value));
  EXPECT_EQ(value, "one");
  EXPECT_EQ(map.Size(), 1);

  EXPECT_FALSE(map.TryRemove(1, value));
  EXPECT_FALSE(map.TryRemove(3, value));
}

TEST(HashMapTest, Contains) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");

  EXPECT_TRUE(map.Contains(1));
  EXPECT_FALSE(map.Contains(2));
}

TEST(HashMapTest, Clear) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  map.Clear();
  EXPECT_EQ(map.Size(), 0);
  EXPECT_TRUE(map.Empty());
}

TEST(HashMapTest, ForEach) {
  HashMap<int, std::string> map;
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

TEST(HashMapTest, ForEachEarlyTermination) {
  HashMap<int, std::string> map;
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

TEST(HashMapTest, ToVector) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::vector<std::pair<int, std::string>> result = map.ToVector();
  EXPECT_EQ(result.size(), 2);
}

TEST(HashMapTest, Keys) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");
  map.Insert(3, "three");

  std::vector<int> keys = map.Keys();
  EXPECT_EQ(keys.size(), 3);
}

TEST(HashMapTest, Values) {
  HashMap<int, std::string> map;
  map.Insert(1, "one");
  map.Insert(2, "two");

  std::vector<std::string> values = map.Values();
  EXPECT_EQ(values.size(), 2);
}

TEST(HashMapTest, ConcurrentAccess) {
  HashMap<int, int> map;
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

TEST(HashMapTest, StringKeys) {
  HashMap<std::string, int> map;
  map.Insert("one", 1);
  map.Insert("two", 2);
  map.Insert("three", 3);

  int value;
  EXPECT_TRUE(map.TryGet("two", value));
  EXPECT_EQ(value, 2);

  EXPECT_EQ(map.Size(), 3);
}

TEST(HashMapTest, MoveOnlyType) {
  HashMap<int, std::unique_ptr<std::string>> map;
  map.Insert(1, std::make_unique<std::string>("one"));
  map.Insert(2, std::make_unique<std::string>("two"));

  EXPECT_EQ(map.Size(), 2);

  std::unique_ptr<std::string> value;
  EXPECT_TRUE(map.TryRemove(1, value));
  EXPECT_EQ(*value, "one");
}
