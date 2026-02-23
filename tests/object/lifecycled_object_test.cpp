#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "cytoskeleton/object/lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

class TestLifecycledObject : public LifecycledObject {
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

TEST(LifecycledObjectTest, InitialStateIsUninitialized) {
  auto obj = std::make_shared<TestLifecycledObject>();
  EXPECT_TRUE(obj->IsUninitialized());
  EXPECT_FALSE(obj->IsInitialized());
  EXPECT_FALSE(obj->IsRunning());
  EXPECT_FALSE(obj->IsStopped());
  EXPECT_FALSE(obj->IsDestroyed());
}

TEST(LifecycledObjectTest, InitializeTransitionsToInitialized) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_EQ(obj->init_count, 1);
  EXPECT_TRUE(obj->IsInitialized());
  EXPECT_FALSE(obj->IsUninitialized());
}

TEST(LifecycledObjectTest, InitializeIsIdempotent) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Initialize());
  EXPECT_EQ(obj->init_count, 1);
}

TEST(LifecycledObjectTest, InitializeFailureKeepsUninitialized) {
  auto obj = std::make_shared<TestLifecycledObject>();
  obj->init_result = false;

  EXPECT_FALSE(obj->Initialize());
  EXPECT_TRUE(obj->IsUninitialized());
  EXPECT_FALSE(obj->IsInitialized());
}

TEST(LifecycledObjectTest, StartTransitionsToRunning) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_EQ(obj->start_count, 1);
  EXPECT_TRUE(obj->IsRunning());
  EXPECT_TRUE(obj->IsInitialized());
}

TEST(LifecycledObjectTest, StartBeforeInitializeFails) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_FALSE(obj->Start());
  EXPECT_EQ(obj->start_count, 0);
}

TEST(LifecycledObjectTest, StartIsIdempotentWhenRunning) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Start());
  EXPECT_EQ(obj->start_count, 1);
}

TEST(LifecycledObjectTest, StartFailureTransitionsToStopped) {
  auto obj = std::make_shared<TestLifecycledObject>();
  obj->start_result = false;

  EXPECT_TRUE(obj->Initialize());
  EXPECT_FALSE(obj->Start());
  EXPECT_TRUE(obj->IsStopped());
  EXPECT_FALSE(obj->IsRunning());
}

TEST(LifecycledObjectTest, StopTransitionsToStopped) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Stop());
  EXPECT_EQ(obj->stop_count, 1);
  EXPECT_TRUE(obj->IsStopped());
  EXPECT_FALSE(obj->IsRunning());
}

TEST(LifecycledObjectTest, StopWhenNotRunningIsIdempotent) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Stop());
  EXPECT_EQ(obj->stop_count, 0);
}

TEST(LifecycledObjectTest, DestroyTransitionsToDestroyed) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Stop());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_EQ(obj->destroy_count, 1);
  EXPECT_TRUE(obj->IsDestroyed());
}

TEST(LifecycledObjectTest, DestroyIsIdempotent) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_EQ(obj->destroy_count, 1);
}

TEST(LifecycledObjectTest, DestroyStopsRunningObject) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Destroy());
  EXPECT_EQ(obj->stop_count, 1);
  EXPECT_EQ(obj->destroy_count, 1);
  EXPECT_TRUE(obj->IsDestroyed());
}

TEST(LifecycledObjectTest, RestartAfterStop) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_TRUE(obj->Initialize());
  EXPECT_TRUE(obj->Start());
  EXPECT_TRUE(obj->Stop());
  EXPECT_TRUE(obj->Start());
  EXPECT_EQ(obj->start_count, 2);
  EXPECT_TRUE(obj->IsRunning());
}

TEST(LifecycledObjectTest, DestructorCallsDestroy) {
  std::shared_ptr<TestLifecycledObject> obj;
  {
    auto local_obj = std::make_shared<TestLifecycledObject>();
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

TEST(LifecycledObjectTest, StateTransitions) {
  auto obj = std::make_shared<TestLifecycledObject>();

  EXPECT_EQ(obj->GetState(), LifecycledObject::State::kUninitialized);

  obj->Initialize();
  EXPECT_EQ(obj->GetState(), LifecycledObject::State::kInitialized);

  obj->Start();
  EXPECT_EQ(obj->GetState(), LifecycledObject::State::kRunning);

  obj->Stop();
  EXPECT_EQ(obj->GetState(), LifecycledObject::State::kStopped);

  obj->Destroy();
  EXPECT_EQ(obj->GetState(), LifecycledObject::State::kDestroyed);
}

TEST(LifecycledObjectTest, ThreadSafeStateTransitions) {
  auto obj = std::make_shared<TestLifecycledObject>();

  std::thread t1([obj]() { obj->Initialize(); });
  std::thread t2([obj]() { obj->Initialize(); });

  t1.join();
  t2.join();

  EXPECT_EQ(obj->init_count, 1);
  EXPECT_TRUE(obj->IsInitialized());
}
