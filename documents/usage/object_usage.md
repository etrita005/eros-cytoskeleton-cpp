# Object 模块使用文档

## 概述

Object 模块是 Cytoskeleton C++ 基础库的核心组件，提供类似 Java Object 的对象基础功能，包括对象级同步、生命周期管理和单例模式支持。

## 命名空间

```cpp
namespace com::etrita::eros::cytos::object;
```

## 设计核心思想

### 1. 一切皆对象
- 所有高层对象都继承自 `Object` 基类
- 内置线程同步原语（Mutex 和 Event），简化并发控制
- 提供 `GetSharedPtr()` 方法获取安全的 shared_ptr

### 2. 生命周期管理
- 标准化的 9 状态生命周期
- 支持重复启动/停止，适用于服务类对象
- 析构时自动清理资源

### 3. 线程安全
- 所有状态转换操作都是线程安全的
- 使用 `concurrent::Mutex` 保护共享状态
- 使用 `concurrent::AutoResetEvent` 实现通知机制

### 4. 单例模式
- CRTP（Curiously Recurring Template Pattern）实现
- 线程安全的延迟初始化
- 自定义删除器支持 protected 析构函数

## 快速开始

### 引入依赖

```cpp
#include "cytoskeleton/object/object.h"

using namespace com::etrita::eros::cytos::object;
```

### Bazel 配置

```python
# BUILD.bazel
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = ["@cytoskeleton//include/cytoskeleton/object:object"],
)
```

## 核心类层次结构

```
┌─────────────────────────────────────────────────────────────┐
│                        Object                                │
│  - GetSharedPtr()                                           │
│  - Join() / Notify() / ResetNotify()                        │
│  - GetMutex()                                               │
│  - Lock() / Unlock() / TryLock()                            │
├─────────────────────────────────────────────────────────────┤
│                    LifecycleObject                           │
│  - Initialize() / Start() / Stop() / Destroy()              │
│  - OnInitialize() / OnStart() / OnStop() / OnDestroy()      │
│  - 9-State Lifecycle Management                             │
├─────────────────────────────────────────────────────────────┤
│               AutoStartLifecycleObject                       │
│  - Automatic thread management                              │
│  - Create()                                                 │
│  - Run()                                                    │
├─────────────────────────────────────────────────────────────┤
│                     Singleton<T>                             │
│  - Instance()                                               │
│  - Thread-safe lazy initialization                          │
└─────────────────────────────────────────────────────────────┘
```

## 功能模块

### 1. Object 基类

所有对象的基类，提供对象级同步和通知机制。

#### 基本用法

```cpp
#include "cytoskeleton/object/object.h"

class MyObject : public Object {
 public:
  void DoSomething() {
    Lock();
    // 临界区
    Unlock();
  }
};

auto obj = std::make_shared<MyObject>();
```

#### 对象级同步

```cpp
class Counter : public Object {
 public:
  void Increment() {
    Lock();
    ++count_;
    std::cout << "Count: " << count_ << std::endl;
    Unlock();
  }

  int GetCount() const {
    Lock();
    int count = count_;
    Unlock();
    return count;
  }

 private:
  int count_ = 0;
};

// 多线程使用
auto counter = std::make_shared<Counter>();

std::thread t1([counter]() {
  for (int i = 0; i < 1000; ++i) {
    counter->Increment();
  }
});

std::thread t2([counter]() {
  for (int i = 0; i < 1000; ++i) {
    counter->Increment();
  }
});

t1.join();
t2.join();

std::cout << "Final count: " << counter->GetCount() << std::endl;  // 2000
```

#### 通知机制

```cpp
class Task : public Object {
 public:
  void Execute() {
    std::cout << "Task executing..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Task completed!" << std::endl;
    Notify();  // 通知等待者任务完成
  }
};

auto task = std::make_shared<Task>();

// 在另一个线程中等待任务完成
std::thread waiter([task]() {
  std::cout << "Waiting for task..." << std::endl;
  task->Join();  // 等待 Notify()
  std::cout << "Task finished!" << std::endl;
});

// 执行任务
task->Execute();

waiter.join();
```

#### 获取安全的 shared_ptr

```cpp
class MyObject : public Object {
 public:
  void DoWork() {
    // 在对象内部获取自身的 shared_ptr
    auto self = GetSharedPtr();
    // 安全地使用 self，无需担心生命周期问题
  }
};

auto obj = std::make_shared<MyObject>();
auto ptr = obj->GetSharedPtr();  // 获取安全的 shared_ptr
```

### 2. LifecycleObject - 生命周期对象

提供标准化的生命周期管理，适用于服务类对象。

#### 9 状态生命周期

```
Uninitialized → Initializing → Initialized → Starting → Running → 
Stopping → Stopped → Destroying → Destroyed
```

#### 基本用法

```cpp
#include "cytoskeleton/object/lifecycled_object.h"

class DataService : public LifecycleObject {
 protected:
  bool OnInitialize() override {
    std::cout << "Initializing service..." << std::endl;
    // 分配资源、加载配置等
    return true;  // 返回 false 表示初始化失败
  }

  bool OnStart() override {
    std::cout << "Starting service..." << std::endl;
    // 启动处理线程、连接数据库等
    return true;
  }

  bool OnStop() override {
    std::cout << "Stopping service..." << std::endl;
    // 停止处理、断开连接等
    return true;
  }

  bool OnDestroy() override {
    std::cout << "Destroying service..." << std::endl;
    // 释放资源
    return true;
  }

  void OnStateChanged(State old_state, State new_state) override {
    std::cout << "State: " << static_cast<int>(old_state) 
              << " -> " << static_cast<int>(new_state) << std::endl;
  }
};

auto service = std::make_shared<DataService>();

// 完整生命周期
service->Initialize();  // Uninitialized -> Initialized
service->Start();       // Initialized -> Running
// ... 服务运行中 ...
service->Stop();        // Running -> Stopped
service->Destroy();     // Stopped -> Destroyed
```

#### 状态查询

```cpp
auto service = std::make_shared<DataService>();

// 查询状态
if (service->IsUninitialized()) {
  std::cout << "Service not initialized" << std::endl;
}

service->Initialize();

if (service->IsInitialized()) {
  std::cout << "Service initialized" << std::endl;
}

service->Start();

if (service->IsRunning()) {
  std::cout << "Service running" << std::endl;
}

// 获取当前状态
State current = service->GetState();
```

#### 重复启动/停止

```cpp
auto service = std::make_shared<DataService>();

service->Initialize();
service->Start();
service->Stop();

// 可以再次启动
service->Start();
service->Stop();

service->Destroy();
```

#### 状态变化监听

```cpp
class MonitoredService : public LifecycleObject {
 protected:
  bool OnInitialize() override {
    std::cout << "Initializing..." << std::endl;
    return true;
  }

  bool OnStart() override {
    std::cout << "Starting..." << std::endl;
    return true;
  }

  bool OnStop() override {
    std::cout << "Stopping..." << std::endl;
    return true;
  }

  bool OnDestroy() override {
    std::cout << "Destroying..." << std::endl;
    return true;
  }

  void OnStateChanged(State old_state, State new_state) override {
    std::cout << "State changed: " << StateToString(old_state)
              << " -> " << StateToString(new_state) << std::endl;
  }
};
```

### 3. AutoStartLifecycleObject - 自动启动生命周期对象

自动管理后台线程，适用于需要持续运行的后台任务。

#### 继承方式

```cpp
#include "cytoskeleton/object/auto_start_lifecycled_object.h"

class BackgroundWorker : public AutoStartLifecycleObject {
 public:
  std::atomic<int> task_count{0};

 protected:
  void Run(std::stop_token stop_token) override {
    std::cout << "Worker thread started" << std::endl;

    while (!stop_token.stop_requested()) {
      // 模拟处理任务
      ++task_count;
      std::cout << "Processing task #" << task_count.load() << std::endl;

      if (stop_token.stop_requested()) {
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Worker thread stopping" << std::endl;
  }
};

// 创建并自动启动
auto worker = BackgroundWorker::Create<BackgroundWorker>();

if (!worker) {
  std::cerr << "Failed to create worker!" << std::endl;
  return 1;
}

std::cout << "Worker running: " << (worker->IsRunning() ? "Yes" : "No") << std::endl;

// 运行一段时间
std::this_thread::sleep_for(std::chrono::seconds(5));

// 停止
worker->Stop();
std::cout << "Total tasks: " << worker->task_count.load() << std::endl;

// 可以再次启动
worker->Start();
```

#### Lambda 方式

```cpp
std::atomic<int> counter{0};

auto worker = AutoStartLifecycleObject::Create(
    [&counter](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        ++counter;
        std::cout << "Lambda worker: " << counter.load() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
);

// 运行
std::this_thread::sleep_for(std::chrono::seconds(2));

// 停止
worker->Stop();
```

#### 优雅停止

```cpp
class DataProcessor : public AutoStartLifecycleObject {
 protected:
  void Run(std::stop_token stop_token) override {
    std::cout << "Processor started" << std::endl;

    while (!stop_token.stop_requested()) {
      // 检查停止请求
      if (stop_token.stop_requested()) {
        std::cout << "Stop requested, finishing current task..." << std::endl;
        // 完成当前任务
        break;
      }

      // 处理数据
      ProcessData();
    }

    std::cout << "Processor stopped" << std::endl;
  }

 private:
  void ProcessData() {
    // 数据处理逻辑
  }
};
```

### 4. Singleton - 单例模式

线程安全的单例模式实现。

#### 基本用法

```cpp
#include "cytoskeleton/object/singleton.h"

class ConfigManager : public Singleton<ConfigManager> {
 public:
  void Set(const std::string& key, const std::string& value) {
    config_[key] = value;
  }

  std::string Get(const std::string& key) {
    return config_[key];
  }

 private:
  std::unordered_map<std::string, std::string> config_;
};

// 获取单例实例
auto& config = Singleton<ConfigManager>::Instance();
config.Set("host", "localhost");
config.Set("port", "8080");

std::string host = config.Get("host");
```

#### Protected 析构函数

```cpp
class Logger : public Singleton<Logger> {
 public:
  void Log(const std::string& message) {
    std::cout << "[LOG] " << message << std::endl;
  }

 protected:
  ~Logger() override = default;  // Singleton 支持 protected 析构
};

// 使用
Singleton<Logger>::Instance().Log("Application started");
```

#### 延迟初始化

```cpp
class Database : public Singleton<Database> {
 public:
  void Connect() {
    std::cout << "Connecting to database..." << std::endl;
    connected_ = true;
  }

  bool IsConnected() const { return connected_; }

 private:
  bool connected_ = false;
};

// 第一次访问时才创建实例
if (Singleton<Database>::Instance().IsConnected()) {
  // 使用数据库
}
```

## 完整示例

### 服务管理器

```cpp
#include <iostream>
#include <memory>
#include <vector>

#include "cytoskeleton/object/lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

// 数据服务
class DataService : public LifecycleObject {
 protected:
  bool OnInitialize() override {
    std::cout << "[DataService] Initializing..." << std::endl;
    return true;
  }

  bool OnStart() override {
    std::cout << "[DataService] Starting..." << std::endl;
    return true;
  }

  bool OnStop() override {
    std::cout << "[DataService] Stopping..." << std::endl;
    return true;
  }

  bool OnDestroy() override {
    std::cout << "[DataService] Destroying..." << std::endl;
    return true;
  }
};

// 网络服务
class NetworkService : public LifecycleObject {
 protected:
  bool OnInitialize() override {
    std::cout << "[NetworkService] Initializing..." << std::endl;
    return true;
  }

  bool OnStart() override {
    std::cout << "[NetworkService] Starting..." << std::endl;
    return true;
  }

  bool OnStop() override {
    std::cout << "[NetworkService] Stopping..." << std::endl;
    return true;
  }

  bool OnDestroy() override {
    std::cout << "[NetworkService] Destroying..." << std::endl;
    return true;
  }
};

// 服务管理器（单例）
class ServiceManager : public Singleton<ServiceManager> {
 public:
  void AddService(std::shared_ptr<LifecycleObject> service) {
    services_.push_back(service);
  }

  bool StartAll() {
    for (auto& service : services_) {
      if (!service->Start()) {
        return false;
      }
    }
    return true;
  }

  bool StopAll() {
    for (auto& service : services_) {
      if (!service->Stop()) {
        return false;
      }
    }
    return true;
  }

  void InitializeAll() {
    for (auto& service : services_) {
      service->Initialize();
    }
  }

  void DestroyAll() {
    for (auto& service : services_) {
      service->Destroy();
    }
  }

 private:
  std::vector<std::shared_ptr<LifecycleObject>> services_;
};

int main() {
  // 创建服务
  auto data_service = std::make_shared<DataService>();
  auto network_service = std::make_shared<NetworkService>();

  // 注册到管理器
  ServiceManager::Instance().AddService(data_service);
  ServiceManager::Instance().AddService(network_service);

  // 初始化所有服务
  ServiceManager::Instance().InitializeAll();

  // 启动所有服务
  ServiceManager::Instance().StartAll();

  std::cout << "All services running" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // 停止所有服务
  ServiceManager::Instance().StopAll();

  // 销毁所有服务
  ServiceManager::Instance().DestroyAll();

  return 0;
}
```

### 后台任务处理器

```cpp
#include <atomic>
#include <iostream>
#include <chrono>

#include "cytoskeleton/object/auto_start_lifecycled_object.h"

using namespace com::etrita::eros::cytos::object;

class TaskProcessor : public AutoStartLifecycleObject {
 public:
  std::atomic<int> processed_count{0};

  void SubmitTask(int task_id) {
    Lock();
    task_queue_.push(task_id);
    Unlock();
    Notify();  // 通知工作线程有新任务
  }

  int GetProcessedCount() const {
    return processed_count.load();
  }

 protected:
  void Run(std::stop_token stop_token) override {
    std::cout << "[TaskProcessor] Worker started" << std::endl;

    while (!stop_token.stop_requested()) {
      int task_id;
      
      // 获取任务
      {
        Lock();
        if (task_queue_.empty()) {
          // 等待新任务（带超时）
          Unlock();
          Join(std::chrono::milliseconds(100));
          continue;
        }
        task_id = task_queue_.front();
        task_queue_.pop();
        Unlock();
      }

      // 处理任务
      std::cout << "[TaskProcessor] Processing task #" << task_id << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      ++processed_count;
    }

    std::cout << "[TaskProcessor] Worker stopped, processed " 
              << processed_count.load() << " tasks" << std::endl;
  }

 private:
  std::queue<int> task_queue_;
};

int main() {
  auto processor = TaskProcessor::Create<TaskProcessor>();

  // 提交任务
  for (int i = 0; i < 20; ++i) {
    processor->SubmitTask(i);
  }

  // 等待处理完成
  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "Total processed: " << processor->GetProcessedCount() << std::endl;

  // 停止
  processor->Stop();

  return 0;
}
```

## 最佳实践

### 1. 使用 LifecycleObject 管理服务

```cpp
// 推荐：使用 LifecycleObject 管理生命周期
class MyService : public LifecycleObject {
 protected:
  bool OnInitialize() override { /* 初始化 */ return true; }
  bool OnStart() override { /* 启动 */ return true; }
  bool OnStop() override { /* 停止 */ return true; }
  bool OnDestroy() override { /* 销毁 */ return true; }
};

auto service = std::make_shared<MyService>();
service->Initialize();
service->Start();
service->Stop();
service->Destroy();
```

### 2. 使用 AutoStartLifecycleObject 管理后台线程

```cpp
// 推荐：使用 AutoStartLifecycleObject 自动管理线程
class Worker : public AutoStartLifecycleObject {
 protected:
  void Run(std::stop_token stop_token) override {
    while (!stop_token.stop_requested()) {
      // 工作
    }
  }
};

auto worker = Worker::Create<Worker>();
// 自动启动，无需手动管理
worker->Stop();  // 优雅停止
```

### 3. 使用单例模式管理全局资源

```cpp
// 推荐：使用 Singleton 管理全局资源
class Config : public Singleton<Config> {
 public:
  std::string Get(const std::string& key);
  void Set(const std::string& key, const std::string& value);
};

// 全局访问
auto value = Singleton<Config>::Instance().Get("key");
```

### 4. 使用对象级同步简化并发控制

```cpp
// 推荐：继承 Object 获得内置同步能力
class SharedResource : public Object {
 public:
  void Update(int value) {
    Lock();
    data_ = value;
    Unlock();
  }

  int Get() const {
    Lock();
    int val = data_;
    Unlock();
    return val;
  }

 private:
  int data_ = 0;
};
```

### 5. 正确处理生命周期状态

```cpp
// 推荐：检查状态后再操作
auto service = std::make_shared<MyService>();

if (!service->IsInitialized()) {
  service->Initialize();
}

if (service->IsInitialized() && !service->IsRunning()) {
  service->Start();
}

// 不推荐：不检查状态直接操作
service->Start();  // 可能失败
```

## 相关文档

- [架构设计文档](../architecture/object_requirements.md)
- [示例代码](../../examples/object/)
- [Concurrent 模块使用文档](concurrent_usage.md)
