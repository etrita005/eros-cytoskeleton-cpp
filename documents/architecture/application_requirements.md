# Cytoskeleton C++ 基础库 - Application 模块需求文档

## 1. 概述

### 1.1 模块名称

`application` - 应用模块

### 1.2 命名空间

`com::etrita::eros::cytos::app`

### 1.3 设计目标

Application 是 EROS 中机器人应用程序的运行框架，提供事件驱动的执行模型、标准化的生命周期管理和可组合的功能扩展机制。

核心设计目标：

- 提供事件驱动的执行模型——基于主消息循环，所有业务逻辑和系统事件通过消息驱动
- 提供机器人应用的标准生命周期——采用分层状态机，体现机器人应用的独有特性
- 提供系统事件机制——将 POSIX 信号和机器人系统事件统一纳入消息循环，应用按需处理
- 提供功能组合机制——基于 CRTP Mixin 模式，用户可按需组合 BlackBox、Sentry 等功能
- 提供代码生成支持——基于 application.yaml 清单文件，生成框架代码和服务配置文件
- 面向单进程、单 Application 的简洁设计
- API 设计原则：安全性 > 易用性 > 性能

### 1.4 设计哲学

**简单性优先**：Application 模块只提供机器人应用运行所必需的基础框架，不预设复杂的管理机制。模块加载、进程间通信等能力由上层或其他模块按需组合。

**事件驱动**：Application 的核心执行模型是事件驱动。主消息循环是应用的唯一执行引擎，所有业务逻辑和外部事件均通过消息驱动，避免多线程竞态。

**功能组合优于继承**：Application 采用 CRTP Mixin 模式实现功能组合，用户通过模板参数按需混入 BlackBox、Sentry 等功能，而非通过深层继承体系。

### 1.5 C++ 标准

C++20

### 1.6 命名约定

- 类名：大驼峰命名（PascalCase），如 `Application`、`SystemEvent`
- 方法名：大驼峰命名（PascalCase），如 `OnInitialize`、`Main`
- 参数名：小驼峰命名（camelCase），如 `app_name`、`event_type`
- 枚举值：k 前缀大驼峰命名（kCamelCase），如 `kRunning`、`kFinalized`

---

## 2. 核心概念

### 2.1 Application（应用）

Application 是机器人应用程序的运行框架实例。每个进程有且仅有一个 Application，它提供：

- **主消息循环**：基于 `MessageQueue` 的 Looper，是应用唯一的执行引擎
- **生命周期管理**：标准化的初始化→运行→退出流程
- **系统事件处理**：将 POSIX 信号和机器人系统事件统一纳入消息循环
- **功能组合**：通过 Mixin 模式按需扩展功能

### 2.2 主消息循环

Application 的核心是主消息循环（Main Looper）。主线程的生命周期围绕消息循环展开：

```
主线程执行流程：

  1. 调用 OnInitialize()        ← 主线程直接调用，消息循环尚未启动
  2. 启动主消息循环              ← Looper 开始运行
  3. 投递第一个消息              ← 此消息的执行体为 Main()
  4. 等待主消息循环结束           ← 主线程阻塞在 Looper
  5. 调用 OnExit()             ← 消息循环已结束，主线程直接调用
```

**关键约束：**

- OnInitialize 和 OnExit 由主线程在消息循环外部直接调用，不在消息循环内执行
- Main() 作为第一条消息投递到消息循环中执行，所有后续业务逻辑和事件处理均在消息循环内完成
- 主线程在消息循环期间阻塞，直到消息循环退出
- OnExit 执行时消息循环已完全停止，不再有消息投递

### 2.3 系统事件

系统事件是 Application 从外部接收的异步通知，包括 POSIX 信号和机器人系统自定义事件。

系统事件的核心设计原则：

- **统一投递到主消息循环**：所有系统事件均通过 self-pipe 机制转换为消息，投递到主消息循环中处理，确保处理逻辑运行在主线程，避免信号处理函数中的竞态条件
- **应用按需处理**：Application 不对系统事件做强制响应，用户在 `OnSystemEvent` 回调中按业务需求自行处理
- **不改变生命周期状态**：系统事件本身不触发生命周期状态迁移，应用自行决定是否因系统事件而退出消息循环

**系统事件类型：**

| 事件类型 | 来源 | 说明 |
|---------|------|------|
| SIGINT | POSIX 信号 | 终端中断（Ctrl+C） |
| SIGTERM | POSIX 信号 | 终止信号（systemd stop 等） |
| SIGUSR1 | POSIX 信号 | 用户自定义信号 1 |
| SIGUSR2 | POSIX 信号 | 用户自定义信号 2 |
| 自定义事件 | 系统内部 | 机器人系统自定义事件（如低电量、模式切换等） |

### 2.4 Mixin 功能组合

Application 采用 CRTP Mixin 模式实现功能组合。用户通过模板参数按需混入功能模块，每个 Mixin 可定义自身的 `Initialize` 和 `Finalize` 行为。

**Mixin 执行顺序：**

- Initialize：按 Mixin 声明顺序依次执行
- Finalize：按 Mixin 声明逆序依次执行（类似栈展开）

**Mixin 与 Application 生命周期的关系：**

- Mixin 的 Initialize 在 OnInitialize 阶段执行（由框架调用 InitializeAll）
- Mixin 的 Finalize 在 OnExit 阶段执行（由框架调用 FinalizeAll）

### 2.5 application.yaml 清单

application.yaml 是 Application 的声明式描述文件，用于代码生成器生成框架代码和服务配置文件。

清单包含：

- **身份信息**：应用名称、版本、描述
- **服务配置**：DBus 服务名、运行用户、可执行路径等部署信息
- **代码生成配置**：代码生成器所需的路径和参数

application.yaml 不是运行时配置文件——它是代码生成阶段的输入，生成的代码和配置文件才是运行时使用的。

### 2.6 代码生成

Application 的框架代码（包括生命周期管理、消息循环、信号处理等）和服务配置文件（如 systemd unit、DBus 配置）由代码生成器基于 application.yaml 生成。

用户仅需编写业务逻辑：OnInitialize、Main、OnExit 的实现，以及按需的 Mixin 功能。

---

## 3. 生命周期需求

### 3.1 设计原则

Application 的生命周期采用分层状态机设计，体现机器人应用的独有特性：

| 维度 | 通用桌面应用 | 机器人 Application | 差异说明 |
|------|------------|-------------------|---------|
| 本质 | 功能逻辑载体 | 业务逻辑载体 | 机器人应用直接关联物理行为 |
| 关键动作 | 初始化/运行/退出 | 初始化/运行/优雅停机 | 机器人应用需时间完成当前操作后安全退出 |
| 机器人特性 | 无 | 优雅停机（graceful shutdown） | 机器人应用不能被简单 kill，需有序退出 |
| 错误处理 | 可忽略或崩溃 | 错误隔离与恢复 | 机器人应用的异常可能产生物理后果 |
| 生命周期方向 | 可重启 | 单向，从创建到终止 | 机器人应用不具备"暂停后重启"的语义 |

**机器人应用的独有特性：**

1. **优雅停机**：机器人应用不能被简单终止。收到停机信号后，应用需要完成当前操作、保存任务状态、释放资源后才能安全退出。这是应用自身对停机请求的负责任处理。
2. **事件驱动一切**：机器人应用的所有行为——业务逻辑、信号响应、定时任务——均由消息驱动，主消息循环是唯一的执行引擎。
3. **确定性启动顺序**：机器人应用的启动是严格的线性流程——初始化→运行→退出，不存在 Android 那样的多组件并发启动。
4. **单次运行**：机器人应用的生命周期是单向的，从创建到终止，不需要 Android 那样的 pause/resume/restart 循环。

### 3.2 需求项

- **REQ-3.2.1** Application 必须实现标准生命周期状态，分为 Operational 和 Finalized 两个顶层状态域
- **REQ-3.2.2** Operational 域必须包含以下子状态：Unconfigured → Configuring → Configured → Initializing → Running → Exiting
- **REQ-3.2.3** Application 必须支持 Error 域，包含 RecoverableError 和 FatalError 两个子状态
- **REQ-3.2.4** Application 的生命周期状态迁移必须是单向的（除 Error 恢复外），不支持回退
- **REQ-3.2.5** Application 进入 Finalized 状态后不可再使用
- **REQ-3.2.6** 状态迁移操作必须线程安全

### 3.3 状态机设计

#### 3.3.1 状态层次结构

```
┌─────────────────────────────────────────────────────────────────┐
│                        Top Level State                           │
└─────────────────────────────────────────────────────────────────┘
                               │
         ┌─────────────────────┼─────────────────────┐
         │                     │                     │
         ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  Operational  │   │     Error     │   │   Finalized   │
│  (正常运行)    │   │   (异常状态)   │   │  (终止状态)    │
└───────┬───────┘   └───────┬───────┘   └───────────────┘
        │                     │
        ▼                     ▼
   子状态：              子状态：
┌───────────────┐   ┌───────────────────┐
│ Unconfigured  │   │ RecoverableError  │
└───────┬───────┘   │   (可自动恢复)     │
        │           └─────────┬─────────┘
        ▼                     │
┌───────────────┐             ▼
│ Configuring   │   ┌───────────────────┐
│  (中间状态)    │   │    FatalError     │
└───────┬───────┘   │   (不可恢复)       │
        │           └─────────┬─────────┘
        ▼                     │
┌───────────────┐             │
│  Configured   │             │
└───────┬───────┘             │
        │                     │
        ▼                     │
┌───────────────┐             │
│ Initializing  │             │
│  (中间状态)    │             │
└───────┬───────┘             │
        │                     │
        ▼                     │
┌───────────────┐             │
│   Running     │             │
│  (运行中)      │             │
└───────┬───────┘             │
        │                     │
        ▼                     │
┌───────────────┐             │
│   Exiting     │             │
│  (退出中)      │             │
└───────┬───────┘             │
        │                     │
        ▼                     ▼
┌───────────────┐     ┌───────────────┐
│  Finalized    │     │  Finalized    │
└───────────────┘     └───────────────┘
```

#### 3.3.2 状态定义

**顶层状态：**

| 状态 | 说明 |
|------|------|
| Operational | 正常运行域，包含从创建到退出的所有正常状态 |
| Error | 异常状态域，包含运行期间的错误状态 |
| Finalized | 终止状态，应用已完全退出，不可再使用 |

**Operational 子状态：**

| 状态 | 类型 | 说明 | 场景 |
|------|------|------|------|
| Unconfigured | 原子 | Application 对象已构造，尚未加载配置 | 对象刚创建 |
| Configuring | 中间 | 正在读取 application.yaml、初始化框架内部状态、设置 Mixin | 框架自动执行 |
| Configured | 原子 | 框架配置完成，等待用户初始化 | 框架就绪 |
| Initializing | 中间 | 正在执行 OnInitialize()，包含 Mixin 的 InitializeAll | 用户初始化代码执行中 |
| Running | 原子 | 主消息循环运行中，Main() 及后续消息在此状态下执行 | 应用正常运行 |
| Exiting | 中间 | 正在执行 OnExit()，包含 Mixin 的 FinalizeAll | 应用正在清理资源 |

**Error 子状态：**

| 状态 | 说明 | 恢复方式 | 场景示例 |
|------|------|---------|---------|
| RecoverableError | 可恢复的异常，应用可自行恢复后继续运行 | 自动恢复 | 通信短暂中断、资源暂时不可用 |
| FatalError | 不可恢复的致命错误，应用必须终止 | 进入 Finalized | 核心依赖缺失、配置严重错误 |

### 3.4 状态迁移表

| 当前状态 | 事件 | 中间状态 | 目标状态 |
|---------|------|---------|---------|
| Unconfigured | 启动 | Configuring | Configured / FatalError |
| Configured | 开始初始化 | Initializing | Running / FatalError |
| Running | 退出消息循环 | Exiting | Finalized |
| Running | 可恢复异常 | - | RecoverableError |
| Running | 致命异常 | - | FatalError |
| RecoverableError | 恢复成功 | - | Running |
| RecoverableError | 恢复失败 | - | FatalError |
| FatalError | 确认终止 | Exiting | Finalized |

**迁移说明：**

- Unconfigured → Configuring：框架启动时自动执行
- Configured → Initializing → Running：框架调用 OnInitialize()，成功后启动消息循环
- Running → Exiting → Finalized：消息循环退出后框架调用 OnExit()
- Error 域的恢复由应用自行决定，框架不强制恢复策略

### 3.5 生命周期回调

| 回调 | 调用时机 | 执行线程 | 消息循环状态 | 职责 |
|------|---------|---------|------------|------|
| OnInitialize | Initializing 阶段 | 主线程 | 未启动 | 用户初始化代码：加载配置、建立连接、准备资源 |
| Main | Running 阶段（首条消息） | 主线程 | 已启动 | 业务入口：投递后续消息、注册定时器、启动业务流程 |
| OnSystemEvent | Running 阶段 | 主线程 | 已启动 | 系统事件处理：POSIX 信号、自定义系统事件 |
| OnExit | Exiting 阶段 | 主线程 | 已停止 | 清理代码：释放资源、保存状态、断开连接 |

**回调执行约束：**

- OnInitialize 和 OnExit 在消息循环外部执行，不能投递消息
- Main 和 OnSystemEvent 在消息循环内部执行，可以投递消息
- 所有回调均在主线程执行，无需加锁

### 3.6 优雅停机

机器人应用的停机必须是优雅的——应用收到停机信号后，需要完成当前操作、保存状态、释放资源后才能安全退出。

**优雅停机流程：**

1. 收到停机信号（SIGTERM/SIGINT），SignalHandler 通过 self-pipe 将信号投递到主消息循环
2. 主消息循环将信号作为系统事件派发给 OnSystemEvent
3. 用户在 OnSystemEvent 中决定停机策略：
   - 立即退出：调用消息循环的 Quit，触发 Exiting → Finalized
   - 延迟退出：标记停机请求，完成当前任务后再 Quit
   - 忽略：继续运行

**与通用桌面应用的关键区别：**

1. **单向生命周期**：通用桌面应用可以被暂停、恢复、重启，而 Application 的生命周期是单向的——从创建到终止，不存在"停用后重新激活"的场景。机器人应用要么在运行，要么在退出。
2. **无 Suspend/Resume**：Application 作为单进程单实例的业务实体，不需要挂起和恢复机制。
3. **优雅停机而非强制终止**：机器人应用收到停机信号后不能立即退出，需要完成当前操作后有序退出——这是应用对物理世界负责的表现。

---

## 4. 主消息循环需求

Application 的执行模型基于主消息循环，所有业务逻辑和事件处理均由消息驱动。

### 4.1 需求项

- **REQ-4.1.1** Application 必须创建唯一的主 Looper，基于 `cytos::itc::message_queue` 的 Looper 模型
- **REQ-4.1.2** 主消息循环必须在主线程运行
- **REQ-4.1.3** Main() 必须作为主消息循环的第一条消息投递执行
- **REQ-4.1.4** 主线程必须等待主消息循环结束（阻塞在 Looper）
- **REQ-4.1.5** 主消息循环退出后，主线程调用 OnExit()
- **REQ-4.1.6** 主消息循环必须支持 Quit 操作，使消息循环正常退出
- **REQ-4.1.7** 主消息循环退出时，丢弃消息队列中未处理的消息

### 4.2 执行时序

```
主线程
  │
  ├─► OnInitialize()           ← 主线程直接调用
  │     │
  │     ├─► InitializeAll()     ← 框架调用所有 Mixin 的 Initialize
  │     └─► [用户初始化代码]
  │
  ├─► 启动主消息循环            ← Looper.Run()
  │     │
  │     ├─► [第一条消息] Main()  ← 消息循环内执行
  │     │     │
  │     │     └─► [用户业务代码，投递后续消息]
  │     │
  │     ├─► [后续消息处理]
  │     │     ├─► OnSystemEvent()  ← 系统事件消息
  │     │     ├─► [业务消息]
  │     │     └─► [定时消息]
  │     │
  │     └─► [Quit 被调用]      ← 消息循环退出
  │
  ├─► OnExit()                 ← 主线程直接调用
  │     │
  │     ├─► FinalizeAll()       ← 框架调用所有 Mixin 的 Finalize
  │     └─► [用户清理代码]
  │
  └─► 进程退出
```

### 4.3 MessageQueue 依赖

Application 必须依赖 `cytos::itc::message_queue` 模块：

- 使用 `Looper` 作为主消息循环引擎
- 使用 `Handler` 作为消息处理接口
- 使用 `Message` 作为消息载体
- Main() 通过 `Handler::SendMessage` 或 `Handler::Post` 投递到消息循环

---

## 5. 系统事件需求

### 5.1 需求项

- **REQ-5.1.1** Application 必须集成 SignalHandler，使用 self-pipe 机制将 POSIX 信号转换为消息投递到主消息循环
- **REQ-5.1.2** Application 必须在 OnInitialize 阶段注册默认信号处理（SIGINT、SIGTERM）
- **REQ-5.1.3** 收到信号后，必须将信号编号和来源封装为系统事件，投递到主消息循环
- **REQ-5.1.4** 系统事件必须在主消息循环中通过 OnSystemEvent 回调派发
- **REQ-5.1.5** Application 必须支持自定义系统事件的投递，允许外部代码通过公共接口向主消息循环投递系统事件
- **REQ-5.1.6** OnSystemEvent 回调默认实现为空，用户按需重写
- **REQ-5.1.7** Application 应支持用户注册自定义信号（SIGUSR1、SIGUSR2 等）

### 5.2 SignalHandler 集成

Application 必须集成基于 self-pipe 机制的 SignalHandler：

**self-pipe 机制原理：**

1. SignalHandler 创建一个 pipe（pipefd_[2]）
2. 使用 `sigaction` 注册信号处理函数
3. 信号处理函数（中断上下文）仅将信号编号写入 pipe 的写端——这是信号安全操作
4. SignalHandler 的后台线程通过 `poll`/`ppoll` 监听 pipe 的读端
5. 读到信号后，通过回调通知 Application
6. Application 将信号封装为系统事件消息，投递到主消息循环

**关键约束：**

- 信号处理函数中仅执行 `write(pipefd, &byte, 1)`，这是 async-signal-safe 操作
- 信号的语义处理在主消息循环的主线程中完成，不在信号处理函数中
- SignalHandler 使用独立的后台线程监听 pipe，不阻塞主线程

### 5.3 系统事件数据结构

系统事件必须包含以下信息：

| 字段 | 类型 | 说明 |
|------|------|------|
| type | enum | 事件类型：kSignal / kCustom |
| signal_number | int | 信号编号（type 为 kSignal 时有效） |
| custom_event_id | int | 自定义事件 ID（type 为 kCustom 时有效） |
| timestamp | uint64_t | 事件发生时间戳 |

### 5.4 默认信号处理

Application 框架应提供默认的信号处理行为，用户可通过重写 OnSystemEvent 覆盖：

| 信号 | 默认行为 |
|------|---------|
| SIGINT | 请求消息循环退出（Quit） |
| SIGTERM | 请求消息循环退出（Quit） |
| SIGUSR1 | 无操作 |
| SIGUSR2 | 无操作 |

用户重写 OnSystemEvent 后，完全由用户决定处理策略，框架的默认行为不再生效。

### 5.5 自定义系统事件

除 POSIX 信号外，Application 必须支持自定义系统事件的投递：

- 外部代码可通过 Application 的公共接口投递自定义系统事件
- 自定义系统事件与 POSIX 信号事件使用相同的投递通道（主消息循环）
- 自定义系统事件通过 OnSystemEvent 回调派发，type 为 kCustom

**机器人场景示例：**

| 自定义事件 | 触发源 | 用户处理方式（示例） |
|-----------|--------|-------------------|
| 低电量通知 | 电源管理服务 | 降低运动速度、提前结束任务 |
| 安全状态变更 | 安全系统 | 停止运动控制、保存状态后退出 |
| 模式切换通知 | 任务调度系统 | 切换业务逻辑分支 |
| 网络状态变更 | 网络管理服务 | 切换离线/在线模式 |

---

## 6. Mixin 功能组合需求

### 6.1 需求项

- **REQ-6.1.1** Application 必须采用 CRTP Mixin 模式实现功能组合，用户通过模板参数混入功能模块
- **REQ-6.1.2** 每个 Mixin 必须提供 `Initialize()` 和 `Finalize()` 方法
- **REQ-6.1.3** 框架必须在 OnInitialize 阶段按 Mixin 声明顺序依次调用所有 Mixin 的 Initialize
- **REQ-6.1.4** 框架必须在 OnExit 阶段按 Mixin 声明逆序依次调用所有 Mixin 的 Finalize
- **REQ-6.1.5** Mixin 可通过 CRTP 的 `static_cast<Derived*>(this)` 访问最终派生类的公共接口
- **REQ-6.1.6** Mixin 可通过 C++20 concepts 检测最终派生类是否提供了特定方法，实现可选的功能钩子

### 6.2 Mixin 模式设计

```cpp
// 递归终止基类
template <typename Derived>
class Application<Derived> {
 protected:
  Derived* self() { return static_cast<Derived*>(this); }
  void InitializeAll() {}
  void FinalizeAll() {}
};

// 递归组合：逐层继承 Mixin
template <typename Derived, template <typename> class First,
          template <typename> class... Rest>
class Application<Derived, First, Rest...>
    : public First<Derived>, public Application<Derived, Rest...> {
 public:
  void Run() {
    InitializeAll();
    self()->Main();
    FinalizeAll();
  }

 protected:
  Derived* self() { return static_cast<Derived*>(this); }

  void InitializeAll() {
    First<Derived>::Initialize();
    Application<Derived, Rest...>::InitializeAll();
  }

  void FinalizeAll() {
    Application<Derived, Rest...>::FinalizeAll();
    First<Derived>::Finalize();
  }
};
```

### 6.3 Mixin 示例（仅说明概念，不在本模块中实现）

以下 Mixin 示例仅用于说明组合机制，BlackBox、Sentry 等具体 Mixin 不在本模块范围内实现。

**SystemEventReceiver Mixin：**

- 提供系统事件接收能力
- 在 Initialize 中启动事件监听线程
- 在 Finalize 中停止事件监听线程
- 通过 concepts 检测最终派生类是否实现了 `OnSystemEventReceived`

**BlackBox Mixin（概念示例）：**

- 提供数据记录能力
- 在 Initialize 中打开记录文件
- 在 Finalize 中关闭记录文件

**Sentry Mixin（概念示例）：**

- 提供异常上报能力
- 在 Initialize 中配置上报端点
- 在 Finalize 中刷新待上报数据

**用户使用示例：**

```cpp
class DeliveryApp : public Application<DeliveryApp,
                                        SystemEventReceiver,
                                        BlackBox,
                                        Sentry> {
 public:
  void Main() { /* 业务逻辑 */ }
  void OnSystemEventReceived() { /* 系统事件处理 */ }
};
```

### 6.4 Mixin 执行顺序

对于 `Application<Derived, A, B, C>`：

```
Initialize 顺序：A::Initialize → B::Initialize → C::Initialize
Finalize 顺序：C::Finalize → B::Finalize → A::Finalize
```

Finalize 逆序执行确保资源按依赖关系的逆序释放——后初始化的资源先释放，避免悬挂引用。

---

## 7. application.yaml 清单需求

### 7.1 需求项

- **REQ-7.1.1** Application 必须提供 application.yaml 作为清单文件
- **REQ-7.1.2** application.yaml 必须包含应用身份信息（app_name、version、description）
- **REQ-7.1.3** application.yaml 必须包含服务配置信息（service_name、run_user、exec_path）
- **REQ-7.1.4** application.yaml 的格式必须人机可读可写，使用 YAML 格式
- **REQ-7.1.5** 代码生成器必须基于 application.yaml 生成 Application 框架代码
- **REQ-7.1.6** 代码生成器必须基于 application.yaml 生成服务配置文件（systemd unit、DBus 配置等）

### 7.2 application.yaml 字段定义

```yaml
app_name: delivery_app
version: "1.0.0"
description: "室内配送应用"
service_name: com.etrita.eros.DeliveryApp
run_user: user
exec_path: /opt/eros/bin/delivery_app
```

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| app_name | string | 是 | 应用名称，用于代码生成中的类名和命名空间 |
| version | string | 是 | 语义化版本号 |
| description | string | 否 | 应用描述 |
| service_name | string | 是 | DBus 服务名（用于生成 DBus 配置） |
| run_user | string | 是 | 运行用户（用于生成 systemd unit） |
| exec_path | string | 是 | 可执行程序安装路径（用于生成 systemd unit） |

### 7.3 代码生成

代码生成器基于 application.yaml 和可选的 proto/IDL 定义，生成以下内容：

**生成的框架代码：**

- Application 基类（生命周期管理、消息循环、信号处理）
- Mixin 组合声明
- main() 入口函数

**生成的服务配置文件：**

- systemd unit 文件
- DBus service 配置文件
- DBus 权限配置文件

**用户编写的内容：**

- OnInitialize、Main、OnExit 的实现
- 自定义 Mixin（可选）
- OnSystemEvent 处理逻辑（可选）

---

## 8. 依赖

### 8.1 Cytoskeleton 内部依赖

| 模块 | 用途 |
|------|------|
| `object` | Application 继承 LifecycledObject，使用 Object 基类 |
| `itc/message_queue` | 主消息循环（Looper、Handler、Message） |
| `concurrent` | 线程安全容器、Mutex（SignalHandler 内部使用） |

### 8.2 系统依赖

| 依赖 | 用途 |
|------|------|
| POSIX signal (sigaction) | 信号处理 |
| pipe / poll | self-pipe 机制 |
| `<filesystem>` | 配置文件路径处理 |

---

## 9. 测试要求

### 9.1 生命周期测试

- 测试状态机转换的正确性：Unconfigured → Configuring → Configured → Initializing → Running → Exiting → Finalized
- 测试 OnInitialize 在消息循环启动前执行
- 测试 Main 作为首条消息在消息循环内执行
- 测试 OnExit 在消息循环停止后执行
- 测试 Finalized 后不可再操作

### 9.2 主消息循环测试

- 测试主消息循环正常启动和退出
- 测试 Main() 正确投递到消息循环
- 测试 Quit 操作使消息循环正常退出
- 测试消息循环退出时丢弃未处理消息

### 9.3 系统事件测试

- 测试 SIGINT/SIGTERM 通过 self-pipe 投递到主消息循环
- 测试 OnSystemEvent 回调在主线程执行
- 测试自定义系统事件投递和派发
- 测试用户重写 OnSystemEvent 覆盖默认行为
- 测试 SIGUSR1/SIGUSR2 注册和处理

### 9.4 Mixin 组合测试

- 测试多个 Mixin 按声明顺序 Initialize
- 测试多个 Mixin 按声明逆序 Finalize
- 测试 Mixin 通过 CRTP 访问最终派生类方法
- 测试 Mixin 使用 concepts 检测可选方法

### 9.5 application.yaml 解析测试

- 测试 application.yaml 正确解析为数据结构
- 测试必需字段缺失时报错
- 测试字段类型校验

---

## 10. 文件结构

```
include/cytoskeleton/application/
├── application.h               # Application 基类（CRTP Mixin 组合）
├── app_state.h                 # AppState 枚举定义
├── system_event.h              # SystemEvent 数据结构定义
├── signal_handler.h            # SignalHandler 类定义（self-pipe 机制）
└── app_manifest.h              # AppManifest 数据结构定义

src/application/
├── application.cc              # Application 基类实现
├── signal_handler.cc           # SignalHandler 实现
└── app_manifest.cc             # AppManifest 解析实现

tests/application/
├── application_test.cpp         # Application 生命周期和消息循环测试
├── system_event_test.cpp        # 系统事件测试
├── mixin_test.cpp               # Mixin 组合测试
├── signal_handler_test.cpp      # SignalHandler 测试
└── app_manifest_test.cpp        # AppManifest 解析测试
```

---

## 11. 注意事项

### 11.1 信号安全

- 信号处理函数中仅允许执行 async-signal-safe 操作（write pipe）
- 信号的语义处理必须在主消息循环的主线程中完成
- 禁止在信号处理函数中调用任何非信号安全函数（包括 malloc、printf、mutex 等）

### 11.2 主线程约束

- OnInitialize 和 OnExit 在主线程执行，但不在消息循环内——不能投递消息
- Main 和 OnSystemEvent 在主线程且在消息循环内——可以投递消息
- 所有生命周期回调均在主线程执行，无需加锁

### 11.3 生命周期单向性

- Application 的生命周期是单向的：Unconfigured → ... → Finalized
- 不支持回退（如从 Running 回到 Configured）
- 不支持重启（Finalized 后不可再使用）
- 机器人应用的运行语境是"从启动到退出"，不存在"暂停后重新启动"的语义

### 11.4 错误处理

- RecoverableError 允许应用自行恢复后继续运行
- FatalError 表示不可恢复的错误，应用必须终止
- Error 域不设专用安全状态——安全类事件通过系统事件机制投递，由用户在 OnSystemEvent 中按需处理

### 11.5 简单性原则

- Application 模块仅提供机器人应用运行所需的基础框架
- 不包含 Module 管理、进程间通信、OTA 等上层能力
- 上层能力通过 Mixin 组合或其他模块按需集成
- 保持单进程、单 Application 的简洁设计

### 11.6 Mixin 设计约束

- Mixin 通过 CRTP 访问最终派生类，不使用虚函数
- Mixin 的 Initialize/Finalize 由框架统一调用，不由用户直接调用
- 具体 Mixin（BlackBox、Sentry 等）不在本模块中实现，由其他模块或用户自行开发
