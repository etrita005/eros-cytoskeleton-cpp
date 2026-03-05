# Cytoskeleton ITC Message Queue 模块

基于消息的消息循环机制，参考 Android Handler 机制和 Windows 消息循环设计。

## 概述

Message Queue 模块提供了一个线程安全的消息传递和处理系统，适用于：

- **跨线程通信**：安全的在多个线程之间传递消息和数据
- **事件驱动架构**：基于消息的事件处理系统
- **UI 消息循环**：类似 Android/Windows 的消息循环模式
- **服务间通信**：模块化服务之间的松耦合通信

## 命名空间

```cpp
namespace com::etrita::eros::cytos::itc::message_queue;
```

## 核心组件

```
┌─────────────────────────────────────────────────────────────┐
│                        Message                               │
│  - what: 消息类型标识                                        │
│  - wparam, lparam: 消息参数                                  │
│  - when: 执行时间                                            │
├─────────────────────────────────────────────────────────────┤
│                        Handler                               │
│  - LambdaHandler: Lambda 函数处理器                          │
│  - ClassHandler: 类处理器                                    │
│  - LightweightHandler: 轻量级处理器                          │
├─────────────────────────────────────────────────────────────┤
│                      MessageQueue                            │
│  - 线程安全的消息队列                                        │
│  - 支持优先级和延迟消息                                      │
├─────────────────────────────────────────────────────────────┤
│                         Looper                               │
│  - 消息循环调度器                                            │
│  - 管理 Handler 和 MessageQueue                              │
│  - 支持主线程 Looper                                         │
└─────────────────────────────────────────────────────────────┘
```

## 快速开始

### 引入依赖

```cpp
#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;
```

### Bazel 配置

```python
# BUILD.bazel
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = ["@cytoskeleton//include/cytoskeleton/itc/message_queue:message_queue"],
)
```

### 基本示例

```cpp
#include <iostream>
#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;

int main() {
  // 创建并启动 Looper
  auto looper = std::make_shared<Looper>("MyLooper", true);

  // 注册 Handler
  looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
    std::cout << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")" << std::endl;
  });

  // 发送消息
  looper->Post<ClickMessage>(100, 200);

  // 等待消息处理完成
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 停止 Looper
  looper->Exit();

  return 0;
}
```

## 详细使用

### 1. Message 消息

消息是消息队列中的基本数据单元。

#### 基本消息

```cpp
// 创建空消息
auto msg = std::make_shared<Message>();

// 创建带类型标识的消息
auto msg = std::make_shared<Message>(MSG_TYPE_CLICK);

// 创建带参数的消息
auto wparam = std::make_shared<MyObject>(data);
auto lparam = std::make_shared<MyObject>(data);
auto msg = std::make_shared<Message>(WM_CLICK, wparam, lparam);
```

#### 自定义消息类型

```cpp
// 定义点击消息
class ClickMessage : public Message {
 public:
  ClickMessage(int x, int y) : x_(x), y_(y) {}
  
  int GetX() const { return x_; }
  int GetY() const { return y_; }
  
 private:
  int x_;
  int y_;
};

// 定义数据消息
class DataMessage : public Message {
 public:
  explicit DataMessage(const std::string& data) : data_(data) {}
  
  const std::string& GetData() const { return data_; }
  
 private:
  std::string data_;
};

// 使用
auto click_msg = std::make_shared<ClickMessage>(100, 200);
auto data_msg = std::make_shared<DataMessage>("Hello");
```

#### 消息属性

```cpp
auto msg = std::make_shared<Message>(MSG_TYPE_DATA);

// 设置参数
msg->SetWParam(std::make_shared<MyObject>(data1));
msg->SetLParam(std::make_shared<MyObject>(data2));

// 获取参数
auto wparam = msg->GetWParam();
auto lparam = msg->GetLParam();

// 设置执行时间（延迟消息）
msg->SetWhen(GetCurrentTimeMs() + 1000);  // 1 秒后执行

// 等待消息处理完成
msg->Join();
msg->Join(std::chrono::milliseconds(500));  // 带超时
```

### 2. Handler 处理器

Handler 负责处理特定类型的消息。

#### Lambda Handler（推荐）

```cpp
// 注册 Lambda Handler
looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
  std::cout << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")" << std::endl;
});

// 注册异步 Handler（在非 UI 线程执行）
looper->RegisterHandler<DataMessage>(
  [](std::shared_ptr<DataMessage> msg) {
    // 耗时操作
    ProcessData(msg->GetData());
  },
  /* async = */ true
);
```

#### 类 Handler

```cpp
// 定义类 Handler
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

#### 轻量级 Handler（Windows 风格）

```cpp
constexpr int WM_CLICK = 1;

looper->RegisterHandler(WM_CLICK, [](object::Object::Ptr wparam, object::Object::Ptr lparam) {
  if (wparam) {
    auto point = std::static_pointer_cast<Point>(wparam);
    std::cout << "Click at (" << point->GetX() << ", " << point->GetY() << ")" << std::endl;
  }
});
```

#### 多个 Handler 处理同一消息类型

```cpp
// 可以为同一消息类型注册多个 Handler
looper->RegisterHandler<DataMessage>([](std::shared_ptr<DataMessage> msg) {
  std::cout << "Handler 1: " << msg->GetData() << std::endl;
});

looper->RegisterHandler<DataMessage>([](std::shared_ptr<DataMessage> msg) {
  std::cout << "Handler 2: " << msg->GetData() << std::endl;
});

// 消息会被所有注册的 Handler 处理
looper->Post<DataMessage>(std::string("Hello"));
```

### 3. Looper 消息循环

Looper 是消息循环的核心，负责管理 Handler 和消息队列。

#### 创建和启动

```cpp
// 创建并自动启动
auto looper = std::make_shared<Looper>("MyLooper", true);

// 创建但不自动启动
auto looper = std::make_shared<Looper>("MyLooper");
looper->AsyncLoop();  // 在新线程中启动

// 停止 Looper
looper->Exit();
```

#### 主线程 Looper

```cpp
// 获取主线程 Looper（自动创建）
auto main_looper = Looper::GetMainLooper();

// 停止主线程 Looper
Looper::StopMainLooper();
```

#### 同步执行

```cpp
// 在 Looper 线程中同步执行
looper->Invoke([]() {
  std::cout << "Running on Looper thread" << std::endl;
});

// 带返回值的同步执行
auto result = looper->Invoke([]() {
  return ComputeResult();
});
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
// 500ms 后执行
looper->PostDelayed<ClickMessage>(500, 100, 200);

// 1 秒后执行
looper->PostDelayed<DataMessage>(1000, std::string("Hello"));
```

#### 同步发送（Invoke）

```cpp
// 同步执行，等待处理完成
looper->Invoke<ClickMessage>(100, 200);

// 带返回值
auto result = looper->Invoke<DataMessage, int>(std::string("data"));
```

## 完整示例

### 示例 1：基本消息处理

```cpp
#include <iostream>
#include <string>
#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;

// 定义消息类型
class TextMessage : public Message {
 public:
  explicit TextMessage(const std::string& text) : text_(text) {}
  const std::string& GetText() const { return text_; }
 private:
  std::string text_;
};

int main() {
  // 创建 Looper
  auto looper = std::make_shared<Looper>("TextLooper", true);

  // 注册 Handler
  looper->RegisterHandler<TextMessage>([](std::shared_ptr<TextMessage> msg) {
    std::cout << "Received: " << msg->GetText() << std::endl;
  });

  // 发送消息
  looper->Post<TextMessage>(std::string("Hello, World!"));
  looper->Post<TextMessage>(std::string("Message Queue Example"));

  // 等待处理完成
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 停止
  looper->Exit();

  return 0;
}
```

### 示例 2：跨线程通信

```cpp
#include <iostream>
#include <atomic>
#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;

class CountMessage : public Message {
 public:
  explicit CountMessage(int count) : count_(count) {}
  int GetCount() const { return count_; }
 private:
  int count_;
};

int main() {
  std::atomic<int> counter{0};
  auto looper = std::make_shared<Looper>("CounterLooper", true);

  // 注册计数器 Handler
  looper->RegisterHandler<CountMessage>([&counter](std::shared_ptr<CountMessage> msg) {
    counter = msg->GetCount();
    std::cout << "Counter updated: " << counter.load() << std::endl;
  });

  // 在工作线程中发送消息
  std::thread worker([&looper]() {
    for (int i = 0; i < 10; ++i) {
      looper->Post<CountMessage>(i * 10);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  worker.join();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  looper->Exit();

  return 0;
}
```

### 示例 3：延迟消息

```cpp
#include <iostream>
#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;

class TimerMessage : public Message {
 public:
  explicit TimerMessage(int timer_id) : timer_id_(timer_id) {}
  int GetTimerId() const { return timer_id_; }
 private:
  int timer_id_;
};

int main() {
  auto looper = std::make_shared<Looper>("TimerLooper", true);

  // 注册定时器 Handler
  looper->RegisterHandler<TimerMessage>([](std::shared_ptr<TimerMessage> msg) {
    std::cout << "Timer " << msg->GetTimerId() << " triggered!" << std::endl;
  });

  // 发送延迟消息
  std::cout << "Scheduling timers..." << std::endl;
  looper->PostDelayed<TimerMessage>(1000, 1);   // 1 秒后
  looper->PostDelayed<TimerMessage>(2000, 2);   // 2 秒后
  looper->PostDelayed<TimerMessage>(3000, 3);   // 3 秒后

  // 等待所有定时器完成
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));

  looper->Exit();

  return 0;
}
```

### 示例 4：主线程 Looper

```cpp
#include <iostream>
#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;

class WorkMessage : public Message {
 public:
  explicit WorkMessage(int work_id) : work_id_(work_id) {}
  int GetWorkId() const { return work_id_; }
 private:
  int work_id_;
};

int main() {
  // 获取主线程 Looper
  auto main_looper = Looper::GetMainLooper();

  // 注册 Handler
  main_looper->RegisterHandler<WorkMessage>([](std::shared_ptr<WorkMessage> msg) {
    std::cout << "Processing work #" << msg->GetWorkId() << std::endl;
  });

  // 从工作线程发送到主线程
  std::thread worker([]() {
    for (int i = 0; i < 5; ++i) {
      Looper::GetMainLooper()->Post<WorkMessage>(i);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 停止主线程 Looper
    Looper::StopMainLooper();
  });

  worker.join();

  return 0;
}
```

## 最佳实践

### 1. 使用 Lambda Handler

```cpp
// 推荐：简洁的 Lambda Handler
looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
  ProcessClick(msg);
});

// 不推荐：过度使用类 Handler（除非需要状态）
class ClickHandler : public Handler<ClickMessage> {
  // ...
};
```

### 2. 合理使用异步 Handler

```cpp
// 耗时操作使用异步 Handler
looper->RegisterHandler<DataMessage>(
  [](std::shared_ptr<DataMessage> msg) {
    // 耗时操作
    ProcessData(msg->GetData());
  },
  true  // async = true
);

// UI 更新使用同步 Handler
looper->RegisterHandler<UiMessage>(
  [](std::shared_ptr<UiMessage> msg) {
    UpdateUI(msg->GetData());
  },
  false  // async = false
);
```

### 3. 使用 PostDelayed 实现定时任务

```cpp
// 延迟执行
looper->PostDelayed<TaskMessage>(1000, task_data);

// 周期性任务（递归调度）
class TimerHandler {
 public:
  void Start(std::shared_ptr<Looper> looper) {
    looper_ = looper;
    ScheduleNext();
  }

 private:
  void ScheduleNext() {
    looper_->PostDelayed<TimerMessage>(1000, /* timer_id */ 1);
  }

  std::shared_ptr<Looper> looper_;
};
```

### 4. 正确停止 Looper

```cpp
// 推荐：在适当时机停止 Looper
auto looper = std::make_shared<Looper>("MyLooper", true);

// ... 使用 Looper ...

// 清理时停止
looper->Exit();
looper.reset();
```

### 5. 使用 Invoke 进行同步调用

```cpp
// 需要立即获取结果时使用 Invoke
auto result = looper->Invoke<ComputeMessage, int>(data);

// 不需要结果时使用 Post
looper->Post<ProcessMessage>(data);
```

## 相关文档

- [使用文档](../../../../documents/usage/message_queue_usage.md)
- [架构设计文档](../../../../documents/architecture/message_queue_requirements.md)
- [示例代码](../../../../examples/itc/message_queue/)
- [Concurrent 模块](../../concurrent/README.md)
- [Object 模块](../../object/README.md)
