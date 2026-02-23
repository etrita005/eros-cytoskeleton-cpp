// Copyright (c) 2024 Etrita. All rights reserved.
//
// 例程名称: object_basic_example
// 例程用途: 演示 Object 基类的基本使用，包括对象级同步和通知机制
//
// 功能说明:
//   1. 使用 Lock/Unlock 进行线程同步
//   2. 使用 Notify/Join 实现对象通知机制
//   3. 使用 GetSharedPtr 获取安全的 shared_ptr

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "cytoskeleton/object/object.h"

using namespace com::etrita::eros::cytos::object;

// 计数器类，演示对象级同步
class Counter : public Object {
 public:
  void Increment() {
    Lock();
    ++count_;
    std::cout << "Count: " << count_ << std::endl;
    Unlock();
  }

  int GetCount() const { return count_; }

 private:
  int count_ = 0;
};

// 任务类，演示通知机制
class Task : public Object {
 public:
  void Execute() {
    std::cout << "Task executing..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Task completed!" << std::endl;
    Notify();  // 通知等待者任务完成
  }
};

int main() {
  std::cout << "=== Object Basic Example ===" << std::endl;

  // 示例 1: 对象级同步
  std::cout << "\n[示例 1] 对象级同步 - 多线程计数器" << std::endl;
  {
    auto counter = std::make_shared<Counter>();

    std::thread t1([counter]() {
      for (int i = 0; i < 5; ++i) {
        counter->Increment();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    std::thread t2([counter]() {
      for (int i = 0; i < 5; ++i) {
        counter->Increment();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    t1.join();
    t2.join();

    std::cout << "Final count: " << counter->GetCount() << std::endl;
  }

  // 示例 2: 通知机制
  std::cout << "\n[示例 2] 通知机制 - 等待任务完成" << std::endl;
  {
    auto task = std::make_shared<Task>();

    // 启动任务线程
    std::thread worker([task]() { task->Execute(); });

    // 主线程等待任务完成
    std::cout << "Main thread waiting for task..." << std::endl;
    task->Join();
    std::cout << "Main thread received notification!" << std::endl;

    worker.join();
  }

  // 示例 3: 获取 shared_ptr
  std::cout << "\n[示例 3] 获取 shared_ptr" << std::endl;
  {
    auto obj = std::make_shared<Object>();

    // 从对象内部获取指向自身的 shared_ptr
    std::shared_ptr<Object> ptr = obj->GetSharedPtr();
    std::cout << "Object use_count: " << ptr.use_count() << std::endl;

    // 在 lambda 中安全地捕获 shared_ptr
    auto lambda = [ptr]() {
      std::cout << "Inside lambda, use_count: " << ptr.use_count() << std::endl;
    };
    lambda();
  }

  std::cout << "\n=== Example completed ===" << std::endl;
  return 0;
}
