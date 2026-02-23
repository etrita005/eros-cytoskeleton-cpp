# Cytoskeleton Object 模块

## 概述

Object 模块是 Cytoskeleton C++ 基础库的核心组件，提供类似 Java Object 的对象基础功能，包括对象级同步、生命周期管理和单例模式支持。

## 设计核心思想

### 1. 一切皆对象
- 所有高层对象都继承自 `Object` 基类
- 内置线程同步原语（Mutex 和 Event），简化并发控制
- 提供 `GetSharedPtr()` 方法获取安全的 shared_ptr

### 2. 生命周期管理
- 标准化的 9 状态生命周期：`Uninitialized` → `Initializing` → `Initialized` → `Starting` → `Running` → `Stopping` → `Stopped` → `Destroying` → `Destroyed`
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

## 技术架构

```
┌─────────────────────────────────────────────────────────────┐
│                        Object                                │
│  - GetSharedPtr()                                           │
│  - Join() / Notify() / ResetNotify()                        │
│  - GetMutex()                                               │
│  - Lock() / Unlock() / TryLock()                            │
├─────────────────────────────────────────────────────────────┤
│                    LifecycledObject                          │
│  - Initialize() / Start() / Stop() / Destroy()              │
│  - OnInitialize() / OnStart() / OnStop() / OnDestroy()      │
│  - 9-State Lifecycle Management                             │
├─────────────────────────────────────────────────────────────┤
│               AutoStartLifecycledObject                      │
│  - Automatic thread management                              │
│  - Create()                                                 │
│  - Run()                                                    │
├─────────────────────────────────────────────────────────────┤
│                     Singleton<T>                             │
│  - Instance()                                               │
│  - Thread-safe lazy initialization                          │
└─────────────────────────────────────────────────────────────┘
```

## 使用方法

### Object 基类

```cpp
#include "cytoskeleton/object/object.h"

class MyObject : public com::etrita::eros::cytos::object::Object {
 public:
  void DoSomething() {
    Lock();
    // Critical section
    Unlock();
  }
};

auto obj = std::make_shared<MyObject>();
auto ptr = obj->GetSharedPtr();  // Get safe shared_ptr
obj->Notify();  // Notify waiters
obj->Join();    // Wait for notification
```

### LifecycledObject 生命周期对象

```cpp
#include "cytoskeleton/object/lifecycled_object.h"

class MyService : public com::etrita::eros::cytos::object::LifecycledObject {
 protected:
  bool OnInitialize() override {
    // Initialize resources
    return true;
  }

  bool OnStart() override {
    // Start service
    return true;
  }

  bool OnStop() override {
    // Stop service
    return true;
  }

  bool OnDestroy() override {
    // Destroy resources
    return true;
  }
};

auto service = std::make_shared<MyService>();
service->Initialize();
service->Start();
// Service running...
service->Stop();
service->Destroy();  // Or let destructor handle it
```

### AutoStartLifecycledObject 自启动对象

```cpp
#include "cytoskeleton/object/auto_start_lifecycled_object.h"

// Method 1: Inheritance
class Worker : public com::etrita::eros::cytos::object::AutoStartLifecycledObject {
 protected:
  void Run(std::stop_token stop_token) override {
    while (!stop_token.stop_requested()) {
      // Do work
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
};

auto worker = Worker::Create<Worker>();  // Auto-initialized and started
// ...
worker->Stop();

// Method 2: Lambda
auto worker2 = com::etrita::eros::cytos::object::AutoStartLifecycledObject::Create(
    [](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        // Do work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
// ...
worker2->Stop();
```

### Singleton 单例

```cpp
#include "cytoskeleton/object/singleton.h"

class ConfigManager : public com::etrita::eros::cytos::object::Singleton<ConfigManager> {
  friend class Singleton<ConfigManager>;

 public:
  void LoadConfig() {}

 protected:
  ConfigManager() = default;
  ~ConfigManager() override = default;
};

auto config = ConfigManager::Instance();
config->LoadConfig();
```

## 例程列表

| 例程文件 | 说明 |
|---------|------|
| [examples/object_basic_example.cpp](#) | Object 基类基本使用：同步、通知机制 |
| [examples/lifecycled_object_example.cpp](#) | 生命周期对象：服务管理 |
| [examples/auto_start_example.cpp](#) | 自启动对象：自动线程管理 |
| [examples/singleton_example.cpp](#) | 单例模式：全局配置管理 |

## 注意事项

### 1. 必须使用 shared_ptr
`Object` 继承自 `std::enable_shared_from_this`，必须通过 `std::make_shared` 或 `std::shared_ptr` 创建，否则 `GetSharedPtr()` 会抛出异常。

### 2. 生命周期状态检查
在重写 `OnStart()`、`OnStop()` 等方法时，应该先调用基类方法确保状态正确：
```cpp
bool OnStart() override {
  if (!LifecycledObject::OnStart()) {
    return false;
  }
  // Custom start logic
  return true;
}
```

### 3. 线程安全
- `Initialize()`、`Start()`、`Stop()`、`Destroy()` 都是线程安全的
- 状态查询方法（`IsRunning()` 等）返回的是快照，可能立即过时
- 使用 `GetMutex()` 获取对象级锁进行自定义同步

### 4. 析构顺序
- `LifecycledObject` 析构时会自动调用 `Destroy()`
- `Destroy()` 会自动调用 `Stop()`（如果正在运行）
- 不需要手动调用 `Destroy()`，但显式调用可以更早释放资源

### 5. AutoStartLifecycledObject 使用建议
- 使用 `Create()` 工厂方法创建对象（支持模板类和 Lambda 两种形式）
- 这些方法会自动调用 `Initialize()` 和 `Start()`
- 如果初始化或启动失败，返回 `nullptr`
- 析构前建议显式调用 `Stop()` 避免潜在的竞态条件

### 6. Singleton 限制
- 构造函数和析构函数必须是 `protected`
- 必须声明 `friend class Singleton<T>`
- 单例实例在程序结束时自动销毁

## 依赖

- C++20 标准库
- `concurrent` 模块（Mutex, AutoResetEvent, Thread）

## 命名空间

```cpp
com::etrita::eros::cytos::object
```

## 文件结构

```
include/cytoskeleton/object/
├── object.h                    # Object 基类
├── lifecycled_object.h         # 生命周期对象
├── auto_start_lifecycled_object.h  # 自启动对象
├── singleton.h                 # 单例模板
└── README.md                   # 本文档

tests/object/
├── object_test.cpp             # Object 测试
├── lifecycled_object_test.cpp  # LifecycledObject 测试
├── auto_start_lifecycled_object_test.cpp  # AutoStartLifecycledObject 测试
└── singleton_test.cpp          # Singleton 测试

examples/
├── object_basic_example.cpp    # Object 基本示例
├── lifecycled_object_example.cpp  # 生命周期示例
├── auto_start_example.cpp      # 自启动示例
└── singleton_example.cpp       # 单例示例
```

## API 设计原则

- **安全性 > 易用性 > 性能**
- 面向机器人应用与服务开发
- 不面向高性能场景
- 遵循 Google C++ Style Guide
