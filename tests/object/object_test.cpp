#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "cytoskeleton/object/object.h"

using namespace com::etrita::eros::cytos::object;

class TestObject : public Object {
 public:
  int value = 0;

  void Increment() {
    Lock();
    ++value;
    Unlock();
  }

  bool TryIncrement() {
    if (!TryLock()) {
      return false;
    }
    ++value;
    Unlock();
    return true;
  }
};

TEST(ObjectTest, GetSharedPtrReturnsValidPointer) {
  auto obj = std::make_shared<TestObject>();
  std::shared_ptr<Object> ptr = obj->GetSharedPtr();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ptr.get(), obj.get());
}

TEST(ObjectTest, GetSharedPtrConstReturnsValidPointer) {
  auto obj = std::make_shared<TestObject>();
  std::shared_ptr<const Object> ptr = obj->GetSharedPtr();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ptr.get(), obj.get());
}

TEST(ObjectTest, LockUnlockProtectsData) {
  auto obj = std::make_shared<TestObject>();

  std::thread t1([obj]() {
    for (int i = 0; i < 1000; ++i) {
      obj->Increment();
    }
  });

  std::thread t2([obj]() {
    for (int i = 0; i < 1000; ++i) {
      obj->Increment();
    }
  });

  t1.join();
  t2.join();

  EXPECT_EQ(obj->value, 2000);
}

TEST(ObjectTest, TryLockSucceedsWhenUnlocked) {
  auto obj = std::make_shared<TestObject>();
  EXPECT_TRUE(obj->TryIncrement());
  EXPECT_EQ(obj->value, 1);
}

TEST(ObjectTest, NotifyAndJoin) {
  auto obj = std::make_shared<TestObject>();
  std::atomic<bool> notified{false};

  std::thread notifier([obj, &notified]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    notified.store(true, std::memory_order_release);
    obj->Notify();
  });

  obj->Join();
  EXPECT_TRUE(notified.load(std::memory_order_acquire));
  notifier.join();
}

TEST(ObjectTest, JoinWithTimeoutReturnsTrueWhenNotified) {
  auto obj = std::make_shared<TestObject>();

  std::thread notifier([obj]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    obj->Notify();
  });

  bool result = obj->Join(std::chrono::milliseconds(500));
  EXPECT_TRUE(result);
  notifier.join();
}

TEST(ObjectTest, JoinWithTimeoutReturnsFalseWhenTimeout) {
  auto obj = std::make_shared<TestObject>();
  bool result = obj->Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
}

TEST(ObjectTest, ResetNotifyClearsSignal) {
  auto obj = std::make_shared<TestObject>();

  obj->Notify();
  obj->ResetNotify();

  bool result = obj->Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
}

TEST(ObjectTest, AutoResetAfterJoin) {
  auto obj = std::make_shared<TestObject>();

  obj->Notify();
  obj->Join();

  bool result = obj->Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);
}

TEST(ObjectTest, GetMutexReturnsValidMutex) {
  auto obj = std::make_shared<TestObject>();
  com::etrita::eros::cytos::concurrent::Mutex& mutex = obj->GetMutex();

  // concurrent::Mutex uses recursive_mutex, so same thread can lock multiple times
  mutex.Lock();
  // In recursive mutex, TryLock from same thread will succeed
  EXPECT_TRUE(obj->TryIncrement());
  mutex.Unlock();
  EXPECT_EQ(obj->value, 1);
}
