# Message Queue 使用文档

## 概述

Message Queue 模块提供了基于消息的消息循环机制，参考 Android Handler 机制和 Windows 消息循环设计。核心组件包括：

- **Message**: 消息载体
- **Handler**: 消息处理器
- **Looper**: 消息循环调度器
- **MessageQueue**: 消息队列

## 命名空间

```cpp
namespace com::etrita::eros::cytos::itc::message_queue;
```

## 核心概念

### 1. Message 消息

消息是消息队列中的基本数据单元。

```cpp
#include "cytoskeleton/itc/message_queue/message_queue_all.h"

using namespace com::etrita::eros::cytos::itc::message_queue;

// 创建基本消息
auto msg = std::make_shared<Message>();

// 创建带类型标识的消息
auto msg = std::make_shared<Message>(MSG_TYPE_CLICK);

// 创建带参数的消息（Windows风格）
auto wparam = std::make_shared<MyObject>(data);
auto lparam = std::make_shared<MyObject>(data);
auto msg = std::make_shared<Message>(WM_CLICK, wparam, lparam);
```

#### 自定义消息类型

```cpp
class ClickMessage : public Message {
 public:
  ClickMessage(int x, int y) : Message(MSG_CLICK), x_(x), y_(y) {}
  
  int GetX() const { return x_; }
  int GetY() const { return y_; }
  
 private:
  int x_;
  int y_;
};
```

### 2. Handler 处理器

Handler 负责处理特定类型的消息。

#### 方式一：Lambda Handler（推荐）

```cpp
// 注册 Lambda Handler
looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
  std::cout << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")" << std::endl;
});

// 注册异步 Lambda Handler
looper->RegisterHandler<ClickMessage>(
  [](std::shared_ptr<ClickMessage> msg) {
    // 耗时操作
    ProcessClick(msg);
  },
  /* async = */ true
);
```

#### 方式二：类 Handler

```cpp
class ClickHandler : public Handler<ClickMessage> {
 protected:
  void Handle(std::shared_ptr<ClickMessage> msg) override {
    std::cout << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")" << std::endl;
  }
};

// 注册类 Handler
auto handler = std::make_shared<ClickHandler>();
looper->RegisterHandler<ClickMessage>(handler);
```

#### 轻量级消息 Handler（Windows风格）

```cpp
constexpr int WM_CLICK = 1;

looper->RegisterHandler(WM_CLICK, [](object::Object::Ptr wparam, object::Object::Ptr lparam) {
  if (wparam) {
    auto point = std::static_pointer_cast<Point>(wparam);
    std::cout << "Click at (" << point->GetX() << ", " << point->GetY() << ")" << std::endl;
  }
});
```

### 3. Looper 消息循环

Looper 是消息循环的核心，负责管理 Handler 和消息队列。

```cpp
// 创建并自动启动 Looper
auto looper = std::make_shared<Looper>("MyLooper", /* auto_start = */ true);

// 创建但不自动启动
auto looper = std::make_shared<Looper>("MyLooper");
looper->AsyncLoop();  // 在新线程中启动

// 停止 Looper
looper->Exit();
```

#### 主线程 Looper

```cpp
// 获取/创建主线程 Looper
auto main_looper = Looper::GetMainLooper();

// 停止主线程 Looper
Looper::StopMainLooper();
```

### 4. 发送消息

#### 异步发送（Post）

```cpp
// Post 消息（自动构造）
looper->Post<ClickMessage>(100, 200);

// Post 已有消息对象
auto msg = std::make_shared<ClickMessage>(100, 200);
looper->Post<ClickMessage>(msg);

// Post 轻量级消息
looper->Post(WM_CLICK, wparam, lparam);
```

#### 延迟发送（PostDelayed）

```cpp
// 延迟 1 秒后发送
looper->PostDelayed<ClickMessage>(std::chrono::seconds(1), 100, 200);
```

#### PostHandler（简化 Lambda 发送）

```cpp
// 直接 Post 一个无参数、无返回值的 Lambda
looper->PostHandler([]() {
  std::cout << "Hello World!" << std::endl;
});

// 延迟 1 秒后执行
looper->PostHandler([]() {
  std::cout << "Delayed hello!" << std::endl;
}, std::chrono::seconds(1));
```

#### InvokeHandler（同步 Lambda 执行）

```cpp
// 同步执行 Lambda，阻塞直到执行完成
bool success = looper->InvokeHandler([]() {
  std::cout << "Sync execution!" << std::endl;
});

// 带超时的同步执行
bool success = looper->InvokeHandler([]() {
  std::cout << "Sync with timeout!" << std::endl;
}, std::chrono::milliseconds(1000));
```

#### 同步发送（Send）

```cpp
// 同步发送，阻塞直到消息处理完成
bool success = looper->Send<ClickMessage>(100, 200);

// 带超时的同步发送
bool success = looper->SendWithTimeout<ClickMessage>(
  std::chrono::seconds(5), 100, 200);
```

## 使用示例

### 完整示例

```cpp
#include "cytoskeleton/itc/message_queue/message_queue_all.h"
#include <iostream>

using namespace com::etrita::eros::cytos::itc::message_queue;

// 定义消息类型
constexpr int MSG_UPDATE = 1;

class UpdateMessage : public Message {
 public:
  UpdateMessage(int value) : Message(MSG_UPDATE), value_(value) {}
  int GetValue() const { return value_; }
 private:
  int value_;
};

int main() {
  // 创建 Looper
  auto looper = std::make_shared<Looper>("MainLooper", true);
  
  // 注册 Handler
  looper->RegisterHandler<UpdateMessage>([](std::shared_ptr<UpdateMessage> msg) {
    std::cout << "Received update: " << msg->GetValue() << std::endl;
  });
  
  // 发送消息
  looper->Post<UpdateMessage>(42);
  
  // 同步发送
  looper->Send<UpdateMessage>(100);
  
  // 延迟发送
  looper->PostDelayed<UpdateMessage>(std::chrono::seconds(1), 200);
  
  // 等待处理完成
  std::this_thread::sleep_for(std::chrono::seconds(2));
  
  // 停止 Looper
  looper->Exit();
  
  return 0;
}
```

### 多 Handler 示例

```cpp
// 一个消息类型可以有多个 Handler
looper->RegisterHandler<Message>([](std::shared_ptr<Message> msg) {
  std::cout << "Handler 1: " << msg->GetWhat() << std::endl;
});

looper->RegisterHandler<Message>([](std::shared_ptr<Message> msg) {
  std::cout << "Handler 2: " << msg->GetWhat() << std::endl;
});

// 发送的消息会被所有 Handler 处理
looper->Post<Message>();
// 输出:
// Handler 1: 0
// Handler 2: 0
```

### Handler 注销

```cpp
// 注册 Handler 并获取 ID
auto handler_id = looper->RegisterHandler<Message>(
  [](std::shared_ptr<Message> msg) { /* ... */ });

// 注销指定 Handler
looper->UnregisterHandler<Message>(handler_id);

// 注销某类型的所有 Handler
looper->UnregisterAllHandlers<Message>();
```

## 线程安全

- `RegisterHandler`、`UnregisterHandler` 方法线程安全
- `Post`、`Send` 方法线程安全
- `Exit` 方法线程安全

## 性能考虑

- 消息队列使用单调时钟（`std::chrono::steady_clock`）避免时间同步影响
- 异步 Handler 使用线程池并发执行
- 默认线程池大小为 `std::thread::hardware_concurrency()`

## API 参考

### Message

| 方法 | 说明 |
|------|------|
| `Message()` | 默认构造函数 |
| `Message(int what)` | 带类型标识的构造函数 |
| `Message(int what, Object::Ptr wparam, Object::Ptr lparam)` | 完整构造函数 |
| `GetWhat()` / `SetWhat(int)` | 获取/设置消息类型 |
| `GetWParam()` / `SetWParam(Object::Ptr)` | 获取/设置 WPARAM |
| `GetLParam()` / `SetLParam(Object::Ptr)` | 获取/设置 LPARAM |
| `GetWhen()` / `SetWhen(uint64_t)` | 获取/设置执行时间 |
| `Join()` | 阻塞等待消息处理完成 |
| `Join(std::chrono::milliseconds timeout)` | 带超时的等待 |
| `Notify()` | 通知消息处理完成 |

### Looper

| 方法 | 说明 |
|------|------|
| `Looper(const std::string& name)` | 构造函数 |
| `Looper(const std::string& name, bool auto_start)` | 带自动启动的构造函数 |
| `GetMainLooper(bool auto_start = true)` | 获取主线程 Looper |
| `StopMainLooper()` | 停止主线程 Looper |
| `RegisterHandler<MessageType>(handler)` | 注册类 Handler |
| `RegisterHandler<MessageType>(func, async)` | 注册 Lambda Handler |
| `RegisterHandler(what, func, async)` | 注册轻量级消息 Handler |
| `UnregisterHandler<MessageType>(id)` | 注销 Handler |
| `UnregisterAllHandlers<MessageType>()` | 注销某类型的所有 Handler |
| `Post<MessageType>(args...)` | 异步发送消息（自动构造） |
| `Post<MessageType>(message)` | 异步发送消息（已有对象） |
| `PostDelayed<MessageType>(delay, args...)` | 延迟发送 |
| `Send<MessageType>(args...)` | 同步发送（阻塞等待） |
| `Send<MessageType>(message)` | 同步发送（已有对象） |
| `SendWithTimeout<MessageType>(timeout, args...)` | 带超时的同步发送 |
| `Post(what, wparam, lparam)` | 发送轻量级消息 |
| `Send(what, wparam, lparam)` | 同步发送轻量级消息 |
| `PostHandler(func, delay)` | 直接发送 Lambda（无需注册 Handler） |
| `InvokeHandler(func)` | 同步执行 Lambda（阻塞等待） |
| `InvokeHandler(func, timeout)` | 带超时的同步执行 Lambda |
| `AsyncLoop()` | 在新线程中启动消息循环 |
| `Loop()` | 消息循环主体（阻塞调用） |
| `Exit()` | 退出消息循环 |
| `IsRunning()` | 是否正在运行 |

## 设计原则

1. **安全性 > 易用性 > 性能**
2. 面向机器人应用与服务开发，不面向高性能场景
3. 支持 Lambda 简化使用
4. 支持同步调用（类似 C# Invoke）
5. 支持延迟执行（基于单调时间）
6. 支持多 Handler 消息分发
