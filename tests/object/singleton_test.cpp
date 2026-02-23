#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "cytoskeleton/object/singleton.h"

using namespace com::etrita::eros::cytos::object;

class TestSingleton : public Singleton<TestSingleton> {
  friend class Singleton<TestSingleton>;

 public:
  int value = 42;

 protected:
  TestSingleton() = default;
  ~TestSingleton() override = default;
};

class AnotherSingleton : public Singleton<AnotherSingleton> {
  friend class Singleton<AnotherSingleton>;

 public:
  int value = 100;

 protected:
  AnotherSingleton() = default;
  ~AnotherSingleton() override = default;
};

TEST(SingletonTest, InstanceReturnsSameObject) {
  auto instance1 = TestSingleton::Instance();
  auto instance2 = TestSingleton::Instance();

  EXPECT_EQ(instance1.get(), instance2.get());
}

TEST(SingletonTest, InstanceReturnsValidObject) {
  auto instance = TestSingleton::Instance();

  EXPECT_NE(instance, nullptr);
  EXPECT_EQ(instance->value, 42);
}

TEST(SingletonTest, DifferentSingletonsAreIndependent) {
  auto test_instance = TestSingleton::Instance();
  auto another_instance = AnotherSingleton::Instance();

  EXPECT_NE(static_cast<void*>(test_instance.get()),
            static_cast<void*>(another_instance.get()));
  EXPECT_EQ(test_instance->value, 42);
  EXPECT_EQ(another_instance->value, 100);
}

TEST(SingletonTest, ThreadSafeInitialization) {
  constexpr int kNumThreads = 10;
  std::vector<std::shared_ptr<TestSingleton>> instances(kNumThreads);
  std::vector<std::thread> threads;

  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&instances, i]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      instances[i] = TestSingleton::Instance();
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  for (int i = 1; i < kNumThreads; ++i) {
    EXPECT_EQ(instances[0].get(), instances[i].get());
  }
}

TEST(SingletonTest, SharedPtrReferenceCounting) {
  std::weak_ptr<TestSingleton> weak_ref;

  {
    auto instance = TestSingleton::Instance();
    weak_ref = instance;
    EXPECT_FALSE(weak_ref.expired());
  }

  auto instance2 = TestSingleton::Instance();
  EXPECT_FALSE(weak_ref.expired());
}
