#include <gtest/gtest.h>

#include <boost/property_tree/ptree.hpp>
#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/tree.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(TreeTest, PutAndGet) {
  Tree tree;

  boost::property_tree::ptree value;
  value.put("", "hello");
  tree.Put("key", value);

  boost::property_tree::ptree result = tree.Get("key");
  EXPECT_EQ(result.get<std::string>(""), "hello");
}

TEST(TreeTest, HasPath) {
  Tree tree;

  boost::property_tree::ptree value;
  value.put("", "test");
  tree.Put("existing_key", value);

  EXPECT_TRUE(tree.HasPath("existing_key"));
  EXPECT_FALSE(tree.HasPath("non_existing_key"));
}

TEST(TreeTest, Remove) {
  Tree tree;

  boost::property_tree::ptree value;
  value.put("", "test");
  tree.Put("key", value);

  EXPECT_TRUE(tree.HasPath("key"));

  tree.Remove("key");
  EXPECT_FALSE(tree.HasPath("key"));
}

TEST(TreeTest, Clear) {
  Tree tree;

  boost::property_tree::ptree value;
  value.put("", "test");
  tree.Put("key1", value);
  tree.Put("key2", value);

  tree.Clear();
  EXPECT_FALSE(tree.HasPath("key1"));
  EXPECT_FALSE(tree.HasPath("key2"));
}

TEST(TreeTest, ForEach) {
  Tree tree;

  boost::property_tree::ptree value1;
  value1.put("", "value1");
  tree.Put("key1", value1);

  boost::property_tree::ptree value2;
  value2.put("", "value2");
  tree.Put("key2", value2);

  int count = 0;
  tree.ForEach([&count](const std::string& key,
                        const boost::property_tree::ptree& value) {
    (void)key;
    (void)value;
    ++count;
    return true;
  });

  EXPECT_EQ(count, 2);
}

TEST(TreeTest, ForEachEarlyTermination) {
  Tree tree;

  boost::property_tree::ptree value;
  value.put("", "test");
  tree.Put("key1", value);
  tree.Put("key2", value);
  tree.Put("key3", value);

  int count = 0;
  tree.ForEach([&count](const std::string& key,
                        const boost::property_tree::ptree& value) {
    (void)key;
    (void)value;
    ++count;
    return count < 2;
  });

  EXPECT_EQ(count, 2);
}

TEST(TreeTest, NestedPaths) {
  Tree tree;

  boost::property_tree::ptree child;
  child.put("name", "child_node");
  tree.Put("parent.child", child);

  boost::property_tree::ptree result = tree.Get("parent.child");
  EXPECT_EQ(result.get<std::string>("name"), "child_node");
}

TEST(TreeTest, ConcurrentAccess) {
  Tree tree;
  const int kNumThreads = 10;
  const int kElementsPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&tree, i]() {
      for (int j = 0; j < kElementsPerThread; ++j) {
        std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
        boost::property_tree::ptree value;
        value.put("", key);
        tree.Put(key, value);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  for (int i = 0; i < kNumThreads; ++i) {
    for (int j = 0; j < kElementsPerThread; ++j) {
      std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
      EXPECT_TRUE(tree.HasPath(key));
    }
  }
}

TEST(TreeTest, ComplexDataTypes) {
  Tree tree;

  boost::property_tree::ptree person;
  person.put("name", "John");
  person.put("age", 30);
  person.put("city", "New York");

  tree.Put("person", person);

  boost::property_tree::ptree result = tree.Get("person");
  EXPECT_EQ(result.get<std::string>("name"), "John");
  EXPECT_EQ(result.get<int>("age"), 30);
  EXPECT_EQ(result.get<std::string>("city"), "New York");
}
