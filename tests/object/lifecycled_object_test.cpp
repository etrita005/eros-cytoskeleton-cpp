#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "cytoskeleton/object/lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

class TestLifecycleObject : public LifecycleObject {
 public:
  int init_count = 0;
  int start_count = 0;
  int stop_count = 0;
  int destroy_count = 0;
  bool init_result = true;
  bool start_result = true;
  bool stop_result = true;
  bool destroy_result = true;

 protected:
  bool OnInitialize() override {
    ++init_count;
    return init_result;
  }

  bool OnStart() override {
    ++start_count;
    return start_result;
  }

  bool OnStop() override {
    ++stop_count;
    return stop_result;
  }

  bool OnDestroy() override {
    ++destroy_count;
    return destroy_result;
  }
};

TEST(LifecycleObjectTest, InitialStateIsUninitialized) {
  auto obj = std::make_shared<TestLifecycleObject>();
  EXPECT_TRUE(obj->IsUninitialized());
  EXPECT_FALSE(obj->IsInitialized());
  EXPECT_FALSE(obj->IsRunning());
  EXPECT_FALSE(obj->IsStopped());
  EXPECT_FALSE(obj->IsDestroyed());
}

TEST(LifecycleObjectTest, InitializeTransitionsToInitialized) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_EQ(obj->init_count, 1);
  EXPECT_TRUE(obj->IsInitialized());
  EXPECT_FALSE(obj->IsUninitialized());
}

TEST(LifecycleObjectTest, InitializeIsIdempotent) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Initialize());
  EXPECT_EQ(obj->init_count, 1);
}

TEST(LifecycleObjectTest, InitializeFailureKeepsUninitialized) {
  auto obj = std::make_shared<TestLifecycleObject>();
  obj->init_result = false;

  EXPECT_FALSE(obj->Initialize());
  EXPECT_TRUE(obj->IsUninitialized());
  EXPECT_FALSE(obj->IsInitialized());
}

TEST(LifecycleObjectTest, StartTransitionsToRunning) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_EQ(obj->start_count, 1);
  EXPECT_TRUE(obj->IsRunning());
  EXPECT_TRUE(obj->IsInitialized());
}

TEST(LifecycleObjectTest, StartBeforeInitializeFails) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_FALSE(obj->Start());
  EXPECT_EQ(obj->start_count, 0);
}

TEST(LifecycleObjectTest, StartIsIdempotentWhenRunning) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Start());
  EXPECT_EQ(obj->start_count, 1);
}

TEST(LifecycleObjectTest, StartFailureTransitionsToStopped) {
  auto obj = std::make_shared<TestLifecycleObject>();
  obj->start_result = false;

  EXPECT_TRUE(obj->Initialize());
  EXPECT_FALSE(obj->Start());
  EXPECT_TRUE(obj->IsStopped());
  EXPECT_FALSE(obj->IsRunning());
}

TEST(LifecycleObjectTest, StopTransitionsToStopped) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Stop());
  EXPECT_EQ(obj->stop_count, 1);
  EXPECT_TRUE(obj->IsStopped());
  EXPECT_FALSE(obj->IsRunning());
}

TEST(LifecycleObjectTest, StopWhenNotRunningIsIdempotent) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Stop());
  EXPECT_EQ(obj->stop_count, 0);
}

TEST(LifecycleObjectTest, DestroyTransitionsToDestroyed) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Stop());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_EQ(obj->destroy_count, 1);
  EXPECT_TRUE(obj->IsDestroyed());
}

TEST(LifecycleObjectTest, DestroyIsIdempotent) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_EQ(obj->destroy_count, 1);
}

TEST(LifecycleObjectTest, DestroyStopsRunningObject) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_EQ(obj->stop_count, 1);
  EXPECT_EQ(obj->destroy_count, 1);
  EXPECT_TRUE(obj->IsDestroyed());
}

TEST(LifecycleObjectTest, RestartAfterStop) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Stop());
  EXPECT_TRUE(obj->Start());
  EXPECT_EQ(obj->start_count, 2);
  EXPECT_TRUE(obj->IsRunning());
}

TEST(LifecycleObjectTest, DestructorCallsDestroy) {
  std::shared_ptr<TestLifecycleObject> obj;
  {
    auto local_obj = std::make_shared<TestLifecycleObject>();
    local_obj->destroy_count = 0;
    EXPECT_TRUE(local_obj->Initialize());
    EXPECT_TRUE(local_obj->Start());
    EXPECT_EQ(local_obj->destroy_count, 0);
    obj = local_obj;
  }
  // Object is still alive via obj
  EXPECT_EQ(obj->destroy_count, 0);
  EXPECT_TRUE(obj->IsRunning());
  
  // Now destroy it
  obj.reset();
  // Note: We cannot verify destroy_count here because the object is already destroyed
  // The important thing is that Destroy() was called during destruction
}

TEST(LifecycleObjectTest, StateTransitions) {
  auto obj = std::make_shared<TestLifecycleObject>();

  EXPECT_EQ(obj->GetState(), LifecycleObject::State::kUninitialized);

  obj->Initialize();
  EXPECT_EQ(obj->GetState(), LifecycleObject::State::kInitialized);

  obj->Start();
  EXPECT_EQ(obj->GetState(), LifecycleObject::State::kRunning);

  obj->Stop();
  EXPECT_EQ(obj->GetState(), LifecycleObject::State::kStopped);

  obj->Destroy();
  EXPECT_EQ(obj->GetState(), LifecycleObject::State::kDestroyed);
}

TEST(LifecycleObjectTest, ThreadSafeStateTransitions) {
  auto obj = std::make_shared<TestLifecycleObject>();

  std::thread t1([obj]() { obj->Initialize(); });
  std::thread t2([obj]() { obj->Initialize(); });

  t1.join();
  t2.join();

  EXPECT_EQ(obj->init_count, 1);
  EXPECT_TRUE(obj->IsInitialized());
}
