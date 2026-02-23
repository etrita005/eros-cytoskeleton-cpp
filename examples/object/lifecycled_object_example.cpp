// Copyright (c) 2024 Etrita. All rights reserved.
//
// 例程名称: lifecycled_object_example
// 例程用途: 演示 LifecycledObject 的生命周期管理，包括服务启动/停止/销毁
//
// 功能说明:
//   1. 实现自定义服务类，重写生命周期回调
//   2. 演示完整生命周期：Initialize -> Start -> Stop -> Destroy
//   3. 演示重复启动/停止功能
//   4. 演示析构时自动清理

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "cytoskeleton/object/lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

// 数据处理器服务，演示生命周期管理
class DataProcessor : public LifecycledObject {
 public:
  int processed_count = 0;

 protected:
  bool OnInitialize() override {
    std::cout << "[DataProcessor] Initializing..." << std::endl;
    // 分配资源、加载配置等
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "[DataProcessor] Initialized successfully" << std::endl;
    return true;
  }

  bool OnStart() override {
    std::cout << "[DataProcessor] Starting..." << std::endl;
    // 启动处理线程、连接数据库等
    running_ = true;
    std::cout << "[DataProcessor] Started successfully" << std::endl;
    return true;
  }

  bool OnStop() override {
    std::cout << "[DataProcessor] Stopping..." << std::endl;
    // 停止处理、断开连接等
    running_ = false;
    std::cout << "[DataProcessor] Stopped successfully" << std::endl;
    return true;
  }

  bool OnDestroy() override {
    std::cout << "[DataProcessor] Destroying..." << std::endl;
    // 释放资源
    std::cout << "[DataProcessor] Destroyed successfully" << std::endl;
    return true;
  }

  void OnStateChanged(State old_state, State new_state) override {
    std::cout << "[DataProcessor] State changed: " << static_cast<int>(old_state)
              << " -> " << static_cast<int>(new_state) << std::endl;
  }

 private:
  bool running_ = false;
};

int main() {
  std::cout << "=== LifecycledObject Example ===" << std::endl;

  // 示例 1: 完整生命周期
  std::cout << "\n[示例 1] 完整生命周期" << std::endl;
  {
    auto processor = std::make_shared<DataProcessor>();

    std::cout << "Initial state: " << (processor->IsUninitialized() ? "Uninitialized" : "Other")
              << std::endl;

    // 初始化
    if (processor->Initialize()) {
      std::cout << "Initialized: " << (processor->IsInitialized() ? "Yes" : "No") << std::endl;
    }

    // 启动
    if (processor->Start()) {
      std::cout << "Running: " << (processor->IsRunning() ? "Yes" : "No") << std::endl;
    }

    // 模拟工作
    std::cout << "Processing data..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 停止
    if (processor->Stop()) {
      std::cout << "Stopped: " << (processor->IsStopped() ? "Yes" : "No") << std::endl;
    }

    // 销毁
    if (processor->Destroy()) {
      std::cout << "Destroyed: " << (processor->IsDestroyed() ? "Yes" : "No") << std::endl;
    }
  }

  // 示例 2: 重复启动/停止
  std::cout << "\n[示例 2] 重复启动/停止" << std::endl;
  {
    auto processor = std::make_shared<DataProcessor>();
    processor->Initialize();

    // 第一次启动
    std::cout << "First start:" << std::endl;
    processor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor->Stop();

    // 第二次启动
    std::cout << "\nSecond start:" << std::endl;
    processor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor->Stop();

    processor->Destroy();
  }

  // 示例 3: 自动销毁
  std::cout << "\n[示例 3] 析构时自动销毁" << std::endl;
  {
    auto processor = std::make_shared<DataProcessor>();
    processor->Initialize();
    processor->Start();
    std::cout << "Service is running..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Leaving scope, destructor will cleanup automatically" << std::endl;
  }

  std::cout << "\n=== Example completed ===" << std::endl;
  return 0;
}
