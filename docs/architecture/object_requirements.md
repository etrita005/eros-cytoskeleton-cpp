# Cytoskeleton C++ 基础库 - Object 模块需求文档

## 1. 概述

### 1.1 模块名称
`object` - 对象基础模块

### 1.2 命名空间
`com::etrita::eros::cytos::object`

### 1.3 设计目标
- 提供类似 Java Object 的基础类，作为所有高层对象的基类
- 内置线程同步原语（Mutex 和 Event），简化对象级并发控制
- 提供对象生命周期管理框架（LifecycledObject）
- 提供单例模式模板（Singleton）
- 提供自启动生命周期对象（AutoStartLifecycledObject）
- API 风格参考 C# 和 Android，遵循 Google C++ Style Guide
- **面向机器人应用与服务开发，不面向高性能场景**
- **API 设计原则：安全性 > 易用性 > 性能**

### 1.4 C++ 标准
C++20

### 1.5 命名约定
- 类名：大驼峰命名（PascalCase），如 `Object`、`LifecycledObject`
- 方法名：大驼峰命名（PascalCase），如 `Initialize`、`GetSharedPtr`
- 参数名：小驼峰命名（camelCase），如 `timeout_ms`

---

## 2. 基础对象 Object

### 2.1 设计说明

`Object` 类作为所有高层对象的基类，提供以下核心功能：
- 内置 `Mutex` 用于对象级同步
- 内置 `ManualResetEvent` 用于对象通知机制
- 提供 `GetSharedPtr()` 方法获取指向自身的 `std::shared_ptr`

**实现说明：**
- 使用 `std::enable_shared_from_this` 实现 `GetSharedPtr()`
- 要求对象必须通过 `std::shared_ptr` 管理生命周期
- 禁止直接构造，必须通过工厂方法或 `std::make_shared` 创建

### 2.2 API

- `Object()` - 构造函数
- `virtual ~Object()` - 虚析构函数
- `std::shared_ptr<Object> GetSharedPtr()` - 获取指向自身的 shared_ptr
- `std::shared_ptr<const Object> GetSharedPtr() const` - 获取 const shared_ptr
- `void Wait()` - 等待对象通知（阻塞）
- `bool Wait(std::chrono::milliseconds timeout)` - 等待对象通知（带超时）
- `void Notify()` - 通知所有等待者
- `void ResetNotify()` - 重置通知状态
- `concurrent::Mutex& GetMutex()` - 获取对象级 Mutex
- `concurrent::ManualResetEvent& GetEvent()` - 获取对象级 Event

**保护方法（供子类使用）：**
- `void Lock()` - 加锁
- `void Unlock()` - 解锁
- `bool TryLock()` - 尝试加锁

### 2.3 使用示例

```cpp
// 定义子类
class MyObject : public Object {
 public:
  void DoSomething() {
    Lock();
    // 临界区操作
    Unlock();
  }
};

// 创建对象（必须通过 shared_ptr）
auto obj = std::make_shared<MyObject>();

// 获取 shared_ptr
std::shared_ptr<Object> ptr = obj->GetSharedPtr();

// 等待通知
obj->Wait();

// 通知对象
obj->Notify();
```

---

## 3. 单例模板 Singleton

### 3.1 设计说明

使用 CRTP（Curiously Recurring Template Pattern）实现线程安全的单例模式。

**实现说明：**
- 使用 `std::once_flag` 和 `std::call_once` 保证线程安全的延迟初始化
- 单例实例通过 `std::shared_ptr` 管理，支持自定义删除器
- 禁止拷贝和移动

### 3.2 API

- `static std::shared_ptr<T> Instance()` - 获取单例实例

### 3.3 使用示例

```cpp
// 定义单例类
class MySingleton : public Singleton<MySingleton> {
  // 声明友元以允许基类访问构造函数
  friend class Singleton<MySingleton>;

 public:
  void DoSomething() {}

 private:
  MySingleton() = default;
  ~MySingleton() override = default;
};

// 使用单例
auto instance = MySingleton::Instance();
instance->DoSomething();
```

---

## 4. 生命周期对象 LifecycledObject

### 4.1 设计说明

`LifecycledObject` 继承自 `Object`，提供标准化的生命周期管理。

**生命周期状态：**
```
Uninitialized -> Initializing -> Initialized -> Starting -> Running -> Stopping -> Stopped -> Destroying -> Destroyed
```

**状态转换规则：**
- `Initialize()`：Uninitialized -> Initializing -> Initialized
- `Start()`：Initialized -> Starting -> Running
- `Stop()`：Running -> Stopping -> Stopped
- `Destroy()`：Stopped -> Destroying -> Destroyed
- 支持重复启动/停止：Stopped -> Starting -> Running

**线程安全：**
- 所有状态转换操作都是线程安全的
- 使用内置 Mutex 保护状态机
- 防止重复调用（幂等性检查）

### 4.2 API

**枚举：**
- `State::kUninitialized` - 未初始化
- `State::kInitializing` - 初始化中
- `State::kInitialized` - 已初始化
- `State::kStarting` - 启动中
- `State::kRunning` - 运行中
- `State::kStopping` - 停止中
- `State::kStopped` - 已停止
- `State::kDestroying` - 销毁中
- `State::kDestroyed` - 已销毁

**公共方法：**
- `LifecycledObject()` - 构造函数
- `~LifecycledObject()` - 析构函数
- `bool Initialize()` - 初始化
- `bool Start()` - 启动
- `bool Stop()` - 停止
- `bool Destroy()` - 销毁
- `State GetState() const` - 获取当前状态
- `bool IsUninitialized() const` - 是否未初始化
- `bool IsInitialized() const` - 是否已初始化
- `bool IsRunning() const` - 是否运行中
- `bool IsStopped() const` - 是否已停止
- `bool IsDestroyed() const` - 是否已销毁

**保护方法（供子类重写）：**
- `virtual bool OnInitialize()` - 初始化回调
- `virtual bool OnStart()` - 启动回调
- `virtual bool OnStop()` - 停止回调
- `virtual bool OnDestroy()` - 销毁回调
- `virtual void OnStateChanged(State old_state, State new_state)` - 状态变化回调

### 4.3 使用示例

```cpp
class MyService : public LifecycledObject {
 public:
  MyService() = default;
  ~MyService() override = default;

 protected:
  bool OnInitialize() override {
    // 初始化资源
    return true;
  }

  bool OnStart() override {
    // 启动服务
    return true;
  }

  bool OnStop() override {
    // 停止服务
    return true;
  }

  bool OnDestroy() override {
    // 销毁资源
    return true;
  }
};

// 使用
auto service = std::make_shared<MyService>();
if (service->Initialize()) {
  if (service->Start()) {
    // 服务运行中
    service->Stop();
  }
  service->Destroy();
}
```

---

## 5. 自启动生命周期对象 AutoStartLifecycledObject

### 5.1 设计说明

`AutoStartLifecycledObject` 继承自 `LifecycledObject`，在 `Start()` 时自动创建线程执行 `Run()` 方法。

**实现说明：**
- 内部使用 `concurrent::Thread` 管理线程
- `Start()` 启动线程，`Stop()` 请求停止并等待线程结束
- 支持重复启动/停止
- 线程函数自动处理 `std::stop_token`

### 5.2 API

- `AutoStartLifecycledObject()` - 默认构造函数（供子类继承使用）
- `explicit AutoStartLifecycledObject(std::function<void(std::stop_token)> run_func)` - Lambda 构造函数
- `~AutoStartLifecycledObject()` - 析构函数

**保护方法（供子类重写）：**
- `virtual void Run(std::stop_token stop_token)` - 线程入口

### 5.3 使用示例

**方式 1 - 子类继承：**
```cpp
class Worker : public AutoStartLifecycledObject {
 public:
  Worker() = default;

 protected:
  void Run(std::stop_token stop_token) override {
    while (!stop_token.stop_requested()) {
      // 执行任务
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
};

auto worker = std::make_shared<Worker>();
worker->Initialize();
worker->Start();  // 自动启动线程执行 Run()
// ...
worker->Stop();   // 请求停止并等待线程结束
worker->Destroy();
```

**方式 2 - Lambda：**
```cpp
auto worker = std::make_shared<AutoStartLifecycledObject>(
    [](std::stop_token stop_token) {
      while (!stop_token.stop_requested()) {
        // 执行任务
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });

worker->Initialize();
worker->Start();
// ...
worker->Stop();
worker->Destroy();
```

---

## 6. Concurrent 模块 Event API 修改

### 6.1 修改说明

为了与 Object 模块的 `Notify()` 方法命名保持一致，需要将 Concurrent 模块中 Event 类的相关 API 从 `Set()` 修改为 `Notify()`。

### 6.2 修改范围

**头文件：** `include/cytoskeleton/concurrent/event.h`

**修改内容：**
- `void Set()` -> `void Notify()`
- `bool IsSet() const` -> `bool IsNotified() const`

**影响：**
- 所有使用 `Event::Set()` 的代码需要修改为 `Event::Notify()`
- 文档、测试用例、示例代码同步更新

### 6.3 修改后 API

- `void Notify()` - 设置事件为有信号状态（原 Set）
- `bool IsNotified() const` - 查询当前状态（原 IsSet）

---

## 7. 依赖

- C++20 标准库
- `concurrent` 模块（Mutex, ManualResetEvent, Thread）

---

## 8. 测试要求

### 8.1 Object 类测试
- 测试 `GetSharedPtr()` 正确返回 shared_ptr
- 测试 `Wait()` / `Notify()` / `ResetNotify()` 基本功能
- 测试多线程环境下的通知机制
- 测试内置 Mutex 的同步功能

### 8.2 Singleton 测试
- 测试单例实例的唯一性
- 测试多线程环境下的线程安全创建
- 测试单例生命周期管理

### 8.3 LifecycledObject 测试
- 测试状态机转换的正确性
- 测试重复调用的幂等性
- 测试多线程环境下的状态安全
- 测试重复启动/停止功能

### 8.4 AutoStartLifecycledObject 测试
- 测试自动启动线程功能
- 测试 `Stop()` 正确停止线程
- 测试重复启动/停止功能
- 测试析构时自动清理线程

---

## 9. 文件结构

```
include/cytoskeleton/object/
├── object.h              # Object 基类
├── singleton.h           # 单例模板
├── lifecycled_object.h   # 生命周期对象
└── auto_start_lifecycled_object.h  # 自启动生命周期对象

src/object/
├── object.cpp
├── lifecycled_object.cpp
└── auto_start_lifecycled_object.cpp

tests/object/
├── object_test.cpp
├── singleton_test.cpp
├── lifecycled_object_test.cpp
└── auto_start_lifecycled_object_test.cpp
```
