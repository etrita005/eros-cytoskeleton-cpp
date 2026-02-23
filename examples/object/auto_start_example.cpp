// Copyright (c) 2024 Etrita. All rights reserved.
//
// 例程名称: auto_start_example
// 例程用途: 演示 AutoStartLifecycledObject 的自动线程管理功能
//
// 功能说明:
//   1. 使用继承方式创建后台工作线程
//   2. 使用 Lambda 方式创建后台工作线程
//   3. 演示自动初始化和启动
//   4. 演示线程停止和重启

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>

#include "cytoskeleton/object/auto_start_lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

// 后台任务处理器，演示继承方式
class BackgroundWorker : public AutoStartLifecycledObject {
 public:
  std::atomic<int> task_count{0};

 protected:
  void Run(std::stop_token stop_token) override {
    std::cout << "[BackgroundWorker] Thread started" << std::endl;

    while (!stop_token.stop_requested()) {
      // 模拟处理任务
      ++task_count;
      std::cout << "[BackgroundWorker] Processing task #" << task_count.load() << std::endl;

      // 检查是否应该停止
      if (stop_token.stop_requested()) {
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[BackgroundWorker] Thread stopping, processed " << task_count.load()
              << " tasks" << std::endl;
  }
};

int main() {
  std::cout << "=== AutoStartLifecycledObject Example ===" << std::endl;

  // 示例 1: 继承方式
  std::cout << "\n[示例 1] 继承方式创建后台线程" << std::endl;
  {
    std::cout << "Creating worker..." << std::endl;
    auto worker = BackgroundWorker::Create<BackgroundWorker>();

    if (!worker) {
      std::cerr << "Failed to create worker!" << std::endl;
      return 1;
    }

    std::cout << "Worker created and auto-started" << std::endl;
    std::cout << "Running: " << (worker->IsRunning() ? "Yes" : "No") << std::endl;

    // 让线程运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "\nStopping worker..." << std::endl;
    worker->Stop();
    std::cout << "Stopped: " << (worker->IsStopped() ? "Yes" : "No") << std::endl;
    std::cout << "Total tasks processed: " << worker->task_count.load() << std::endl;
  }

  // 示例 2: Lambda 方式
  std::cout << "\n[示例 2] Lambda 方式创建后台线程" << std::endl;
  {
    std::atomic<int> counter{0};

    std::cout << "Creating lambda worker..." << std::endl;
    auto worker = AutoStartLifecycledObject::Create(
        [&counter](std::stop_token stop_token) {
          std::cout << "[LambdaWorker] Thread started" << std::endl;

          while (!stop_token.stop_requested()) {
            ++counter;
            std::cout << "[LambdaWorker] Counter: " << counter.load() << std::endl;

            if (stop_token.stop_requested()) {
              break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
          }

          std::cout << "[LambdaWorker] Thread stopping" << std::endl;
        });

    if (!worker) {
      std::cerr << "Failed to create worker!" << std::endl;
      return 1;
    }

    std::cout << "Lambda worker created and auto-started" << std::endl;

    // 让线程运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\nStopping lambda worker..." << std::endl;
    worker->Stop();
    std::cout << "Final counter: " << counter.load() << std::endl;
  }

  // 示例 3: 重启线程
  std::cout << "\n[示例 3] 重启线程" << std::endl;
  {
    std::atomic<int> total_count{0};

    auto worker = AutoStartLifecycledObject::Create(
        [&total_count](std::stop_token stop_token) {
          while (!stop_token.stop_requested()) {
            ++total_count;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
          }
        });

    // 第一次运行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "After first run: " << total_count.load() << std::endl;
    worker->Stop();

    // 重启
    std::cout << "Restarting..." << std::endl;
    worker->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "After restart: " << total_count.load() << std::endl;
    worker->Stop();
  }

  std::cout << "\n=== Example completed ===" << std::endl;
  return 0;
}
