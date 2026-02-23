# Cytoskeleton C++ 基础库 - MessageQueue 模块需求文档

## 1. 概述

### 1.1 模块名称
`message_queue` - 消息队列模块

### 1.2 命名空间
`com::etrita::eros::cytos::message_queue`

### 1.3 设计目标
- 提供以消息为中心的消息循环机制，参考 Android Handler 机制和 Windows 消息循环
- 核心三元素：执行者（Handler）、消息循环（Looper）、消息（Message）
- 支持自定义 Message 子类投递，也支持无需定义 Message 子类的轻量消息（类似 Windows 消息循环）
- 支持单一消息多执行者 Handler
- 支持延迟执行消息（基于单调时间实现，避免时间同步影响）
- 支持同步调用（类似 C# Invoke），调用者阻塞等待消息被所有对应 Handler 处理完成后返回
- 支持异步 Handler，异步 Handler 的执行由线程池并发调用
- 支持 Lambda 作为 Handler，简化使用
- API 风格参考 C# 和 Android，遵循 Google C++ Style Guide
- **面向机器人应用与服务开发，不面向高性能场景**
- **API 设计原则：安全性 > 易用性 > 性能**

### 1.4 C++ 标准
C++20

### 1.5 命名约定
- 类名：大驼峰命名（PascalCase），如 `Message`、`Looper`、`Handler`
- 方法名：大驼峰命名（PascalCase），如 `SendMessage`、`PostDelayed`
- 参数名：小驼峰命名（camelCase），如 `delay_ms`

---

## 2. 核心概念

### 2.1 简化架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         Message Loop                            │
│                                                                 │
│   ┌─────────────┐      ┌─────────────┐      ┌─────────────┐    │
│   │   Handler   │      │   Message   │      │    Looper   │    │
│   │  (执行者)    │<────>│   (消息)     │<────>│  (消息循环)  │    │
│   └─────────────┘      └─────────────┘      └─────────────┘    │
│        ↑                                            │           │
│        │                                            │           │
│        └────────────── 注册/发送 ────────────────────┘           │
│                                                                 │
│  Looper 直接管理：                                               │
│  - RegisterHandler<T>() - 按消息类型注册 Handler                 │
│  - Post<T>() / Send<T>() - 发送消息                              │
│  - 支持 Lambda 作为 Handler                                      │
└─────────────────────────────────────────────────────────────────┘
```

**Handler（执行者）**
- 负责处理特定类型的消息
- 可以是类（继承 Handler<T>）或 Lambda 函数
- 支持同步和异步两种执行模式

**Message（消息）**
- 消息载体，包含消息类型、数据等信息
- 支持自定义 Message 子类扩展
- 支持轻量级消息（类似 Windows 消息循环的 MSG 结构）

**Looper（消息循环）**
- 消息队列的管理者和调度者
- 维护消息类型到 Handler 列表的映射
- 维护消息优先级队列（按执行时间排序）
- 提供直接的注册和发送 API

---

## 3. Message 消息

### 3.1 设计说明

`Message` 是消息的基类，提供基础的消息属性和方法。

**两种消息使用模式：**

#### 模式一：轻量级消息（Windows 消息循环风格）
类似 Windows 的 `MSG` 结构，无需定义 Message 子类，直接使用 `Message` 基类：
- 通过 `what` 字段标识消息类型（如 `WM_CLICK`, `WM_PAINT`）
- 通过 `wparam` 和 `lparam` 携带数据（Object::Ptr 类型，可传递任意对象引用）
- 适合简单消息场景，无需定义额外类

#### 模式二：自定义 Message 子类
通过继承 `Message` 实现特定业务消息：
- 类型安全，编译期检查
- 可以携带复杂数据结构
- 适合复杂业务场景

**消息属性：**
- `what`：消息类型标识（整数）
- `wparam`：消息参数1（Object::Ptr，类似 Windows WPARAM，但传递对象引用）
- `lparam`：消息参数2（Object::Ptr，类似 Windows LPARAM，但传递对象引用）
- `when`：执行时间戳（基于单调时钟，毫秒级时间戳）
- `event`：同步事件（用于同步调用等待）

### 3.2 API

**类型定义：**
- `using Ptr = std::shared_ptr<Message>` - 消息智能指针类型

**构造函数：**
- `Message()` - 默认构造函数
- `explicit Message(int what)` - 指定消息类型
- `Message(int what, object::Object::Ptr wparam, object::Object::Ptr lparam)` - Windows 风格构造（what + 两个对象参数）

**公共方法：**
- `int GetWhat() const` - 获取消息类型
- `void SetWhat(int what)` - 设置消息类型
- `object::Object::Ptr GetWParam() const` - 获取消息参数1（WPARAM）
- `void SetWParam(object::Object::Ptr wparam)` - 设置消息参数1
- `object::Object::Ptr GetLParam() const` - 获取消息参数2（LPARAM）
- `void SetLParam(object::Object::Ptr lparam)` - 设置消息参数2
- `uint64_t GetWhen() const` - 获取执行时间（毫秒级时间戳）
- `void SetWhen(uint64_t when)` - 设置执行时间
- `void Join()` - 阻塞等待消息处理完成（同步调用使用）
- `bool Join(std::chrono::milliseconds timeout)` - 带超时的等待
- `void Notify()` - 通知消息处理完成

**保护成员（供子类使用）：**
- `int what_` - 消息类型
- `object::Object::Ptr wparam_` - 消息参数1（WPARAM）
- `object::Object::Ptr lparam_` - 消息参数2（LPARAM）
- `uint64_t when_` - 执行时间（毫秒级时间戳，0 表示立即执行）
- `concurrent::ManualResetEvent event_` - 同步事件

### 3.3 使用示例

**模式一：轻量级消息（Windows 消息循环风格）**

```cpp
// 定义消息类型（类似 Windows 的 WM_ 常量）
constexpr int WM_CLICK = 1;
constexpr int WM_KEYDOWN = 2;
constexpr int WM_TIMER = 3;

// 创建轻量级消息（使用 wparam/lparam 携带对象引用）
// 方式1：简单消息
auto msg1 = std::make_shared<Message>(WM_CLICK);

// 方式2：携带对象引用（使用 Object 子类包装数据）
// 假设 Point 是 Object 的子类
auto point = std::make_shared<Point>(100, 200);
auto msg2 = std::make_shared<Message>(WM_CLICK, point, nullptr);

// 方式3：传递任意 Object 对象
auto data = std::make_shared<StringObject>("hello");
auto msg3 = std::make_shared<Message>(WM_DATA, nullptr, data);
```

**模式二：自定义 Message 子类**

```cpp
class ClickMessage : public Message {
 public:
  ClickMessage(int x, int y) : Message(MSG_TYPE_CLICK), x_(x), y_(y) {}
  
  int GetX() const { return x_; }
  int GetY() const { return y_; }
  
 private:
  int x_;
  int y_;
};

// 使用自定义消息
auto msg = std::make_shared<ClickMessage>(100, 200);
```

**两种模式对比：**

| 特性 | 轻量级消息（Windows 风格） | 自定义 Message 子类 |
|------|---------------------------|---------------------|
| 定义方式 | 直接使用 `Message` 基类 | 继承 `Message` 创建子类 |
| 数据携带 | wparam/lparam（Object::Ptr，跨线程传递对象引用） | 任意成员变量（复杂数据） |
| 类型安全 | 运行时检查 | 编译期类型检查 |
| 适用场景 | 简单消息、快速开发 | 复杂业务、类型安全要求高 |
| 性能 | 轻量（无虚函数开销） | 略重（虚函数、动态分配） |

---

## 4. Handler 执行者

### 4.1 设计说明

`Handler` 是消息处理器的基类，采用模板设计，支持两种使用方式：

1. **类方式**：继承 `Handler<T>`，重写 `Run` 方法
2. **Lambda 方式**：直接使用 Lambda 函数，简化使用

**Handler 类型：**
- **同步 Handler**：在 Looper 线程中同步执行消息处理
- **异步 Handler**：由线程池并发执行消息处理

### 4.2 API

**IHandler 接口：**
```cpp
class IHandler {
 public:
  using Ptr = std::shared_ptr<IHandler>;
  virtual ~IHandler() = default;
  virtual void Run(Message::Ptr message) = 0;
  virtual bool IsAsync() const { return false; }
};
```

**Handler 模板类（类方式）：**
```cpp
template <typename MessageType = Message>
class Handler : public IHandler {
 public:
  using Ptr = std::shared_ptr<Handler<MessageType>>;
  
  void Run(Message::Ptr message) override {
    Handle(std::static_pointer_cast<MessageType>(message));
  }
  
 protected:
  virtual void Handle(std::shared_ptr<MessageType> message) = 0;
};
```

**LambdaHandler（Lambda 方式包装器）：**
```cpp
template <typename MessageType = Message>
class LambdaHandler : public Handler<MessageType> {
 public:
  using HandlerFunc = std::function<void(std::shared_ptr<MessageType>)>;
  
  explicit LambdaHandler(HandlerFunc func, bool async = false)
      : func_(std::move(func)), async_(async) {}
  
  void Handle(std::shared_ptr<MessageType> message) override {
    func_(message);
  }
  
  bool IsAsync() const override { return async_; }
  
 private:
  HandlerFunc func_;
  bool async_;
};
```

### 4.3 使用示例

**方式一：类 Handler（传统方式）**
```cpp
class ClickHandler : public Handler<ClickMessage> {
 protected:
  void Handle(std::shared_ptr<ClickMessage> message) override {
    LOG(INFO) << "Click at (" << message->GetX() << ", " << message->GetY() << ")";
  }
};

// 注册
looper->RegisterHandler<ClickMessage>(std::make_shared<ClickHandler>());
```

**方式二：Lambda Handler（简化方式）**
```cpp
// 注册 Lambda Handler（同步）
looper->RegisterHandler<Message>([](std::shared_ptr<Message> msg) {
  LOG(INFO) << "Received message: " << msg->GetWhat();
});

// 注册 Lambda Handler（异步）
looper->RegisterHandler<Message>(
  [](std::shared_ptr<Message> msg) {
    LOG(INFO) << "Async processing: " << msg->GetWhat();
  },
  /* async = */ true
);

// 使用轻量级消息的 Lambda（wparam/lparam 为 Object::Ptr）
looper->RegisterHandler(WM_CLICK, [](object::Object::Ptr wparam, object::Object::Ptr lparam) {
  // 将 Object 转换为具体类型
  auto point = std::dynamic_pointer_cast<Point>(wparam);
  if (point) {
    LOG(INFO) << "Click at (" << point->GetX() << ", " << point->GetY() << ")";
  }
});
```

---

## 5. Looper 消息循环

### 5.1 设计说明

`Looper` 是消息循环的核心类，直接提供 Handler 注册和消息发送 API。

**核心功能：**
- 维护 `std::unordered_map<std::type_index, std::vector<IHandler::Ptr>>` 映射
- 按消息类型管理 Handler 列表
- 支持延迟消息（通过 `when` 时间控制）
- 使用单调时钟（`std::chrono::steady_clock`）避免时间同步影响

**线程模型：**
- Looper 在独立线程中运行消息循环
- 异步 Handler 使用线程池执行
- 支持优雅停止和紧急停止

### 5.2 API

**类型定义：**
- `using HandlerId = uint64_t` - Handler 标识类型

**静态方法：**
- `static std::shared_ptr<Looper> GetMainLooper(bool auto_start = true)` - 获取主线程 Looper
- `static void StopMainLooper()` - 停止主线程 Looper

**构造函数：**
- `explicit Looper(const std::string& name = "UNKNOWN")` - 构造函数
- `explicit Looper(const std::string& name, bool auto_start)` - 带自动启动的构造函数
- `~Looper()` - 析构函数（自动调用 Exit）

**Handler 注册方法：**
- `template <typename MessageType> HandlerId RegisterHandler(std::shared_ptr<Handler<MessageType>> handler)` - 注册类 Handler
- `template <typename MessageType> HandlerId RegisterHandler(std::function<void(std::shared_ptr<MessageType>)> func, bool async = false)` - 注册 Lambda Handler
- `HandlerId RegisterHandler(int what, std::function<void(object::Object::Ptr, object::Object::Ptr)> func, bool async = false)` - 注册轻量级消息 Handler（按 what）
- `template <typename MessageType> void UnregisterHandler(HandlerId id)` - 注销 Handler
- `template <typename MessageType> void UnregisterAllHandlers()` - 注销某类型的所有 Handler

**消息发送方法：**
- `template <typename MessageType, typename... Args> void Post(Args&&... args)` - 异步发送消息（自动创建）
- `template <typename MessageType> void Post(std::shared_ptr<MessageType> message)` - 异步发送消息（已有对象）
- `template <typename MessageType, typename... Args> void PostDelayed(std::chrono::milliseconds delay, Args&&... args)` - 延迟发送
- `template <typename MessageType, typename... Args> bool Send(Args&&... args)` - 同步发送（阻塞等待）
- `template <typename MessageType> bool Send(std::shared_ptr<MessageType> message)` - 同步发送（已有对象）
- `template <typename MessageType, typename... Args> bool SendWithTimeout(std::chrono::milliseconds timeout, Args&&... args)` - 带超时的同步发送
- `void Post(int what, object::Object::Ptr wparam = nullptr, object::Object::Ptr lparam = nullptr)` - 发送轻量级消息
- `bool Send(int what, object::Object::Ptr wparam = nullptr, object::Object::Ptr lparam = nullptr)` - 同步发送轻量级消息

**控制方法：**
- `void AsyncLoop()` - 在新线程中启动消息循环
- `void Loop()` - 消息循环主体（阻塞调用）
- `void Exit()` - 退出消息循环
- `bool IsRunning() const` - 是否正在运行

### 5.3 使用示例

**基本使用（Lambda 方式）：**
```cpp
// 创建并启动 Looper
auto looper = std::make_shared<Looper>("MyLooper", /* auto_start = */ true);

// 注册 Lambda Handler（最简单的方式）
looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
  LOG(INFO) << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")";
});

// 发送消息（自动创建 Message）
looper->Post<ClickMessage>(100, 200);

// 同步发送
bool success = looper->Send<ClickMessage>(100, 200);

// 延迟发送
looper->PostDelayed<ClickMessage>(std::chrono::seconds(1), 100, 200);
```

**轻量级消息（Windows 风格）：**
```cpp
// 注册轻量级消息 Handler
looper->RegisterHandler(WM_CLICK, [](object::Object::Ptr wparam, object::Object::Ptr lparam) {
  auto point = std::dynamic_pointer_cast<Point>(wparam);
  if (point) {
    LOG(INFO) << "Click at (" << point->GetX() << ", " << point->GetY() << ")";
  }
});

// 发送轻量级消息（传递 Object::Ptr）
auto point = std::make_shared<Point>(100, 200);
looper->Post(WM_CLICK, point, nullptr);

// 同步发送
bool success = looper->Send(WM_CLICK, point, nullptr);
```

**多 Handler 支持：**
```cpp
// 一个消息类型可以注册多个 Handler
looper->RegisterHandler<Message>([](std::shared_ptr<Message> msg) {
  LOG(INFO) << "Handler 1: " << msg->GetWhat();
});

looper->RegisterHandler<Message>([](std::shared_ptr<Message> msg) {
  LOG(INFO) << "Handler 2: " << msg->GetWhat();
});

// 发送的消息会被两个 Handler 都处理
looper->Post<Message>(MSG_TYPE_UPDATE);
```

**异步 Handler：**
```cpp
// 注册异步 Handler（在线程池中执行）
looper->RegisterHandler<Message>(
  [](std::shared_ptr<Message> msg) {
    // 耗时操作
    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG(INFO) << "Async processed: " << msg->GetWhat();
  },
  /* async = */ true
);

// 消息会在线程池中并发处理
looper->Post<Message>(MSG_TYPE_HEAVY_TASK);
```

**主线程 Looper：**
```cpp
// 在主线程中获取/创建主 Looper
auto main_looper = Looper::GetMainLooper();

// 在其他线程中向主 Looper 发送消息
std::thread worker([]() {
  auto main = Looper::GetMainLooper();
  main->Post<UpdateMessage>();
});

// 停止主 Looper
Looper::StopMainLooper();
```

---

## 6. MessageQueue 消息队列

### 6.1 设计说明

`MessageQueue` 是消息队列的实现，负责消息的存储和调度。

**特性：**
- 线程安全的链表实现
- 按执行时间排序（延迟消息支持）
- 支持优雅退出

### 6.2 API

**公共方法：**
- `void Clean()` - 清空队列
- `MessagePtr Next()` - 获取下一条可执行的消息（阻塞等待）
- `bool EnqueueMessage(MessagePtr message, uint64_t delay_ms = 0)` - 将消息加入队列
- `void Quit()` - 请求退出
- `void Dump()` - 打印队列内容（调试用）

**内部实现：**
- 使用 `concurrent::List` 存储消息
- 使用 `ManualResetEvent` 实现等待/通知机制
- 使用 `std::chrono::steady_clock` 获取单调时间

---

## 7. 线程安全与并发

### 7.1 线程安全保证

**Looper 线程安全：**
- `RegisterHandler`、`UnregisterHandler` 方法线程安全
- `Post`、`Send` 方法线程安全
- `Exit` 方法线程安全
- 消息队列操作使用 Mutex 保护

**Handler 线程安全：**
- Handler 的执行由 Looper 或线程池调用，实现时需自行保证线程安全

### 7.2 异步 Handler 线程池

**线程池配置：**
- 每个 Looper 拥有独立的线程池
- 默认线程池大小：`std::thread::hardware_concurrency()`
- 异步 Handler 共享同一个 Looper 的线程池

---

## 8. 依赖

- C++20 标准库
- `concurrent` 模块（Mutex, ThreadPool, Thread, Event, List）
- `object` 模块（Object, LifecycledObject）

---

## 9. 测试要求

### 9.1 Message 类测试
- 测试消息创建和属性设置
- 测试自定义 Message 子类
- 测试 wparam/lparam 使用

### 9.2 Handler 类测试
- 测试类 Handler 消息处理
- 测试 Lambda Handler 消息处理
- 测试同步/异步 Handler
- 测试模板类型自动转换

### 9.3 Looper 类测试
- 测试 Handler 注册和注销
- 测试消息循环启动和停止
- 测试消息投递和分发
- 测试延迟消息（验证单调时间）
- 测试同步调用（Send）
- 测试多 Handler 消息分发

### 9.4 MessageQueue 类测试
- 测试消息入队/出队
- 测试延迟消息排序
- 测试优雅退出

### 9.5 并发测试
- 测试多线程投递消息
- 测试异步 Handler 并发执行
- 测试同步调用超时处理
- 测试 Looper 优雅停止

### 9.6 集成测试
- 测试完整消息循环场景
- 测试 Handler 生命周期管理
- 测试 Lambda 和类 Handler 混合使用

---

## 10. 文件结构

```
include/cytoskeleton/message_queue/
├── message.h           # Message 基类
├── handler.h           # Handler 基类、模板和 Lambda 包装器
├── message_queue.h     # MessageQueue 消息队列
└── looper.h            # Looper 消息循环

src/message_queue/
├── message.cpp
├── handler.cpp
├── message_queue.cpp
└── looper.cpp

tests/message_queue/
├── message_test.cpp
├── handler_test.cpp
├── message_queue_test.cpp
├── looper_test.cpp
└── integration_test.cpp
```

---

## 11. 完整使用示例

```cpp
#include <cytoskeleton/message_queue/looper.h>
#include <cytoskeleton/message_queue/handler.h>
#include <cytoskeleton/message_queue/message.h>

using namespace com::etrita::eros::cytos::message_queue;

// 定义消息类型（轻量级消息）
constexpr int WM_CLICK = 1;
constexpr int WM_KEYDOWN = 2;

// 自定义消息（复杂场景）
class ClickMessage : public Message {
 public:
  ClickMessage(int x, int y) : Message(MSG_TYPE_CLICK), x_(x), y_(y) {}
  int GetX() const { return x_; }
  int GetY() const { return y_; }
  
 private:
  int x_;
  int y_;
};

int main() {
  // 创建并启动 Looper
  auto looper = std::make_shared<Looper>("MainLooper", /* auto_start = */ true);
  
  // ========== 方式1：Lambda Handler（最简单） ==========
  
  // 轻量级消息 Handler
  looper->RegisterHandler(WM_CLICK, [](object::Object::Ptr wparam, object::Object::Ptr lparam) {
    auto point = std::dynamic_pointer_cast<Point>(wparam);
    if (point) {
      LOG(INFO) << "Click at (" << point->GetX() << ", " << point->GetY() << ")";
    }
  });
  
  // 自定义消息 Handler（Lambda）
  looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
    LOG(INFO) << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")";
  });
  
  // 异步 Handler（Lambda）
  looper->RegisterHandler<Message>(
    [](std::shared_ptr<Message> msg) {
      LOG(INFO) << "Async processing: " << msg->GetWhat();
    },
    /* async = */ true
  );
  
  // ========== 方式2：类 Handler（传统方式） ==========
  
  class MyHandler : public Handler<ClickMessage> {
   protected:
    void Handle(std::shared_ptr<ClickMessage> msg) override {
      LOG(INFO) << "Class handler: (" << msg->GetX() << ", " << msg->GetY() << ")";
    }
  };
  
  looper->RegisterHandler<ClickMessage>(std::make_shared<MyHandler>());
  
  // ========== 发送消息 ==========
  
  // 发送轻量级消息（无需定义 Message 子类）
  auto point1 = std::make_shared<Point>(100, 200);
  looper->Post(WM_CLICK, point1, nullptr);
  
  auto point2 = std::make_shared<Point>(150, 250);
  looper->Send(WM_CLICK, point2, nullptr);  // 同步发送
  
  // 发送自定义消息
  looper->Post<ClickMessage>(100, 200);
  looper->Send<ClickMessage>(150, 250);  // 同步发送
  
  // 延迟发送
  looper->PostDelayed<ClickMessage>(std::chrono::seconds(1), 200, 300);
  
  // 等待一会儿让消息处理完成
  std::this_thread::sleep_for(std::chrono::seconds(2));
  
  // 停止 Looper
  looper->Exit();
  
  return 0;
}
```

---

## 12. 设计对比

### 12.1 与参考项目对比

| 特性 | 参考项目（有 MessageBuilder） | 本设计（简化版） |
|------|------------------------------|------------------|
| 架构复杂度 | 多一层 MessageBuilder | Looper 直接管理 |
| 使用便捷性 | 需要创建 Builder | 直接调用 Looper API |
| Lambda 支持 | 不支持 | 原生支持 |
| 代码量 | 较多 | 较少 |

### 12.2 与 Android Handler 对比

| 特性 | Android Handler | 本设计 |
|------|-----------------|--------|
| Handler 创建 | 继承 Handler 类 | 支持类和 Lambda |
| 消息发送 | sendMessage/post | Post/Send 模板方法 |
| 延迟消息 | sendMessageDelayed | PostDelayed |
| 同步调用 | 无 | Invoke/Send |
| 多 Handler | 一个 Looper 多个 Handler | 支持，按消息类型分发 |

### 12.3 设计优势

1. **简化架构**：移除 MessageBuilder，Looper 直接提供 API
2. **Lambda 支持**：原生支持 Lambda，简化 Handler 定义
3. **类型安全**：模板参数编译期检查
4. **灵活使用**：支持类 Handler 和 Lambda Handler 混合使用
5. **轻量级消息**：支持 Windows 风格的 wparam/lparam 消息
