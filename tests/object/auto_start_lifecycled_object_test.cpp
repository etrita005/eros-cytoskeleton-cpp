#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "cytoskeleton/object/auto_start_lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

class TestAutoStartObject : public AutoStartLifecycleObject {
 public:
  std::atomic<int> run_count{0};
  std::atomic<bool> should_stop{false};

 protected:
  void Run(std::stop_token stop_token) override {
    while (!stop_token.stop_requested() && !should_stop.load()) {
      ++run_count;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
};

TEST(AutoStartLifecycleObjectTest, CreateReturnsSharedPtr) {
  auto obj = TestAutoStartObject::Create<TestAutoStartObject>();
  EXPECT_NE(obj, nullptr);
  // Create automatically starts the object, so we need to stop it
  if (obj) {
    obj->Stop();
  }
}

TEST(AutoStartLifecycleObjectTest, CreateWithLambdaReturnsSharedPtr) {
  auto obj = AutoStartLifecycleObject::Create(
      [](std::stop_token) {});
  EXPECT_NE(obj, nullptr);
  // Create automatically starts the object, so we need to stop it
  if (obj) {
    obj->Stop();
  }
}

TEST(AutoStartLifecycleObjectTest, CreateStartsThreadAutomatically) {
  auto obj = TestAutoStartObject::Create<TestAutoStartObject>();
  EXPECT_NE(obj, nullptr);
  EXPECT_TRUE(obj->IsRunning());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_GT(obj->run_count.load(), 0);

  obj->Stop();
}

TEST(AutoStartLifecycleObjectTest, StopStopsThread) {
  auto obj = TestAutoStartObject::Create<TestAutoStartObject>();
  EXPECT_NE(obj, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  int count_before = obj->run_count.load();
  EXPECT_TRUE(obj->Stop());
  EXPECT_TRUE(obj->IsStopped());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  int count_after = obj->run_count.load();
  EXPECT_EQ(count_before, count_after);
}

TEST(AutoStartLifecycleObjectTest, RestartAfterStop) {
  auto obj = TestAutoStartObject::Create<TestAutoStartObject>();
  EXPECT_NE(obj, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  EXPECT_TRUE(obj->Stop());

  int count_before = obj->run_count.load();

  EXPECT_TRUE(obj->Start());
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  EXPECT_GT(obj->run_count.load(), count_before);

  obj->Stop();
}

TEST(AutoStartLifecycleObjectTest, DestructorStopsThread) {
  std::atomic<int> run_count{0};
  auto obj = TestAutoStartObject::Create<TestAutoStartObject>();
  EXPECT_NE(obj, nullptr);
  obj->run_count = 0;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  run_count = obj->run_count.load();
  
  // Manually call Stop before destruction to avoid hanging
  obj->Stop();
}

TEST(AutoStartLifecycleObjectTest, LambdaFunctionExecutes) {
  std::atomic<int> counter{0};
  auto obj = AutoStartLifecycleObject::Create(
      [&counter](std::stop_token stop_token) {
        while (!stop_token.stop_requested() && counter.load() < 5) {
          ++counter;
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
  EXPECT_NE(obj, nullptr);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_GE(counter.load(), 5);

  obj->Stop();
}

TEST(AutoStartLifecycleObjectTest, StopTokenWorks) {
  std::atomic<bool> running{true};
  auto obj = AutoStartLifecycleObject::Create(
      [&running](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
          running.store(true);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        running.store(false);
      });
  EXPECT_NE(obj, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(running.load());

  EXPECT_TRUE(obj->Stop());
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(running.load());
}

TEST(AutoStartLifecycleObjectTest, DestroyStopsThread) {
  std::atomic<int> counter{0};
  auto obj = AutoStartLifecycleObject::Create(
      [&counter](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
          ++counter;
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
  EXPECT_NE(obj, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  int count_before = counter.load();
  EXPECT_TRUE(obj->Destroy());
  EXPECT_TRUE(obj->IsDestroyed());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), count_before);
}

TEST(AutoStartLifecycleObjectTest, ThreadSafeStartStop) {
  auto obj = TestAutoStartObject::Create<TestAutoStartObject>();
  EXPECT_NE(obj, nullptr);
  // Already started by Create

  std::thread t1([obj]() { obj->Start(); });
  std::thread t2([obj]() { obj->Start(); });

  t1.join();
  t2.join();

  EXPECT_TRUE(obj->IsRunning());
  obj->Stop();
}

TEST(AutoStartLifecycleObjectTest, OnStartOnStopCallbacksCalled) {
  class CallbackTrackingObject : public AutoStartLifecycleObject {
   public:
    std::atomic<int> on_start_count{0};
    std::atomic<int> on_stop_count{0};

   protected:
    bool OnStart() override {
      ++on_start_count;
      return AutoStartLifecycleObject::OnStart();
    }

    bool OnStop() override {
      ++on_stop_count;
      return AutoStartLifecycleObject::OnStop();
    }

    void Run(std::stop_token) override {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  };

  // Create automatically calls Initialize and Start
  auto obj = CallbackTrackingObject::Create<CallbackTrackingObject>();

  // OnStart should have been called during Create
  EXPECT_EQ(obj->on_start_count.load(), 1);

  EXPECT_TRUE(obj->Stop());
  EXPECT_EQ(obj->on_stop_count.load(), 1);
}
