# Cytoskeleton C++ 基础库 - Time 模块需求文档

## 1. 概述

### 1.1 模块名称

`time` - 时间基础设施模块

### 1.2 命名空间

`com::etrita::eros::cytos::time`

### 1.3 设计目标

- 提供统一、易用的时间基础设施，解决业务代码中时间相关的常见问题
- 明确区分系统时间（WallTime）与单调时间（MonoTime），避免混用导致的逻辑错误
- 统一时间间隔（Duration）表示，消除 timeout 单位混乱问题
- 提供高性能的耗时统计工具（Stopwatch），消除重复的性能统计代码
- 提供 Clock 抽象与 FakeClock 实现，支持测试中的时间 Mock
- 基于 `std::chrono` 实现，提供更直观、更安全的 API 包装，降低用户心智负担
- API 风格参考 C# 和 Android，遵循 Google C++ Style Guide
- **面向机器人应用与服务开发，不面向高性能场景**
- **API 设计原则：安全性 > 易用性 > 性能**

### 1.4 C++ 标准

C++20

### 1.5 命名约定

- 类名：大驼峰命名（PascalCase），如 `Duration`、`WallTime`
- 方法名：大驼峰命名（PascalCase），如 `FromMilliseconds`、`ToSeconds`
- 参数名：小驼峰命名（camelCase），如 `timeout_ms`
- 常量名：kCamelCase，如 `kNanosecondsPerSecond`

### 1.6 实现原则

本模块基于 `std::chrono` 实现，定位为 **降低 std::chrono 心智负担的 API 包装层**：

| 本模块类型      | 内部存储                                    | std::chrono 对应            |
| ---------- | --------------------------------------- | ------------------------- |
| `Duration` | `std::chrono::nanoseconds`              | `duration<int64_t, nano>` |
| `WallTime` | `std::chrono::system_clock::time_point` | 系统时钟时间点                   |
| `MonoTime` | `std::chrono::steady_clock::time_point` | 单调时钟时间点                   |

**为什么包装 std::chrono：**

- `std::chrono` 类型冗长（`std::chrono::steady_clock::time_point`），本模块提供简短类型名
- `std::chrono` 时间点不可直接格式化，本模块提供 `Format()` 等便捷方法
- `std::chrono` 缺少 Clock 抽象，无法在测试中 Mock 时间，本模块提供 `Clock` 接口
- `std::chrono` 单位转换需显式 `duration_cast`，本模块提供 `ToMilliseconds()` 等直接转换
- `std::chrono` 缺少 Stopwatch、Deadline 等常用工具，本模块补齐

***

## 2. 核心概念

### 2.1 时间类型分类

| 类型       | 含义           | 时钟源                         | 典型用途             |
| -------- | ------------ | --------------------------- | ---------------- |
| WallTime | 真实世界时间（墙上时钟） | `std::chrono::system_clock` | 日志时间戳、协议时间、时间格式化 |
| MonoTime | 单调时间点        | `std::chrono::steady_clock` | 耗时测量、超时控制、调度基准   |
| Duration | 时间间隔         | 无（纯数值）                      | 超时参数、延迟、速率计算     |

**设计原则：**

- **WallTime** 可因 NTP 同步、手动调时等原因发生跳变，仅用于"表示时刻"
- **MonoTime** 保证单调递增，用于"测量间隔"和"超时控制"
- **Duration** 是值类型，不依赖任何时钟源，可自由参与算术运算

### 2.2 分层架构

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 3: 工具层 (Utility)                                       │
│                                                                 │
│  ┌──────────────────┐          ┌──────────────────┐            │
│  │    Stopwatch     │          │     Deadline     │            │
│  │   (耗时统计)      │          │   (超时控制)      │            │
│  │                  │          │                  │            │
│  │  Clock* clock_ ──┼──┐       │  Clock* clock_ ──┼──┐        │
│  │  MonoTime start_ │  │       │  MonoTime expire_│  │        │
│  └──────────────────┘  │       └──────────────────┘  │        │
│          │             │              │              │        │
└──────────┼─────────────┼──────────────┼──────────────┼────────┘
           │             │              │              │
           ▼             │              ▼              │
┌─────────────────────────────────────────────────────────────────┐
│  Layer 2: 时间获取层 (Time Acquisition)                           │
│                                                                 │
│  ┌─────────────────────────────────────────────┐               │
│  │           Clock (抽象接口)                    │               │
│  │   GetWallTime() → WallTime                  │               │
│  │   GetMonoTime() → MonoTime                  │               │
│  └─────────────────────────────────────────────┘               │
│        ▲                           ▲                           │
│        │ 实现                       │ 实现                      │
│  ┌─────┴──────────────┐  ┌────────┴──────────────┐            │
│  │   SystemClock      │  │     FakeClock         │            │
│  │  (生产环境)         │  │    (测试环境)          │            │
│  │                    │  │                       │            │
│  │  system_clock::now │  │  Advance / SetWall    │            │
│  │  steady_clock::now │  │  SetMono / AdvanceWall│            │
│  └────────────────────┘  └───────────────────────┘            │
│        ▲                                                       │
│        │ 委托                                                   │
│  ┌─────┴──────────────────────────────────────┐               │
│  │  WallTime::Now()  →  SystemClock.GetWallTime() │               │
│  │  MonoTime::Now()  →  SystemClock.GetMonoTime() │               │
│  └────────────────────────────────────────────┘               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
           │                                             │
           ▼                                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Layer 1: 值类型层 (Value Types)                                  │
│                                                                 │
│  ┌────────────┐  ┌────────────────┐  ┌────────────────┐       │
│  │  Duration  │  │   WallTime     │  │   MonoTime     │       │
│  │ (时间间隔)  │  │  (墙上时钟)     │  │  (单调时钟)     │       │
│  └────────────┘  └────────────────┘  └────────────────┘       │
│                                                                 │
│  算术/比较运算    Format/转换      算术/比较运算                 │
│  FromChrono()    FromChrono()     FromChrono()                 │
│  ToChrono()      ToChrono()       ToChrono()                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
           │              │               │
           ▼              ▼               ▼
┌─────────────────────────────────────────────────────────────────┐
│  Layer 0: 基础层 (Foundation)                                     │
│                                                                 │
│  std::chrono::nanoseconds                                       │
│  std::chrono::system_clock::time_point                          │
│  std::chrono::steady_clock::time_point                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**层次依赖规则：**

- 上层可依赖下层，下层不可依赖上层
- Layer 1（值类型）仅依赖 Layer 0（std::chrono），不依赖 Clock
- Layer 2（时间获取）依赖 Layer 1（产出值类型实例）和 Layer 0
- Layer 3（工具）依赖 Layer 2（Clock）和 Layer 1（MonoTime/Duration）
- `WallTime::Now()` / `MonoTime::Now()` 是便捷 API，委托给 SystemClock

***

## 3. Duration 时间间隔

### 3.1 设计说明

`Duration` 表示时间间隔，内部基于 `std::chrono::nanoseconds` 实现，提供多种单位的工厂方法和转换方法。

**实现说明：**

- 内部存储为 `std::chrono::nanoseconds chrono_duration_`
- 工厂方法返回值类型，无需动态分配
- 支持算术运算（加、减、标量乘除）
- 支持比较运算
- 零值和负值均合法（负值表示"时间倒流"语义，如超时剩余为负）
- 与 `std::chrono::duration` 的互转为零开销（直接访问内部存储）

### 3.2 API

**常量：**

- `static constexpr Duration Zero()` - 零值 Duration

**工厂方法：**

- `static Duration FromNanoseconds(int64_t ns)`
- `static Duration FromMicroseconds(int64_t us)`
- `static Duration FromMilliseconds(int64_t ms)`
- `static Duration FromSeconds(int64_t sec)`
- `static Duration FromSecondsDouble(double sec)`

**转换方法：**

- `int64_t ToNanoseconds() const`
- `int64_t ToMicroseconds() const`
- `int64_t ToMilliseconds() const`
- `int64_t ToSeconds() const`
- `double ToSecondsDouble() const`

**算术运算：**

- `Duration operator+(const Duration& rhs) const`
- `Duration operator-(const Duration& rhs) const`
- `Duration operator*(int64_t scalar) const`
- `Duration operator/(int64_t scalar) const`
- `int64_t operator/(const Duration& rhs) const` - 计算倍数关系
- `Duration operator-() const` - 取反
- `Duration& operator+=(const Duration& rhs)`
- `Duration& operator-=(const Duration& rhs)`

**比较运算：**

- `bool operator==(const Duration& rhs) const`
- `bool operator!=(const Duration& rhs) const`
- `bool operator<(const Duration& rhs) const`
- `bool operator<=(const Duration& rhs) const`
- `bool operator>(const Duration& rhs) const`
- `bool operator>=(const Duration& rhs) const`

**工具方法：**

- `bool IsZero() const`
- `bool IsNegative() const`
- `Duration Abs() const` - 取绝对值

**std::chrono 互转：**

- `static Duration FromChrono(const std::chrono::nanoseconds& chrono_dur)` - 从 chrono duration 构造
- `std::chrono::nanoseconds ToChrono() const` - 转换为 chrono duration（零开销，返回内部存储）

### 3.3 使用示例

```cpp
using namespace com::etrita::eros::cytos::time;

auto d1 = Duration::FromMilliseconds(500);
auto d2 = Duration::FromSeconds(1);

auto total = d1 + d2;
LOG(INFO) << total.ToMilliseconds();  // 1500

auto diff = d2 - d1;
LOG(INFO) << diff.ToMilliseconds();  // 500

auto doubled = d1 * 2;
LOG(INFO) << doubled.ToMilliseconds();  // 1000

if (d1 < d2) {
  LOG(INFO) << "d1 is shorter";
}

// 与 std::chrono 交互（零开销）
auto chrono_dur = std::chrono::seconds(3);
auto my_dur = Duration::FromChrono(chrono_dur);
LOG(INFO) << my_dur.ToMilliseconds();  // 3000

void ThirdPartyFunc(std::chrono::milliseconds timeout);
auto my_timeout = Duration::FromSeconds(2);
ThirdPartyFunc(std::chrono::duration_cast<std::chrono::milliseconds>(my_timeout.ToChrono()));
```

***

## 4. WallTime 墙上时钟时间

### 4.1 设计说明

`WallTime` 表示真实世界时间点，内部基于 `std::chrono::system_clock::time_point` 实现。

**实现说明：**

- 内部存储为 `std::chrono::system_clock::time_point time_point_`
- 适用于日志时间戳、协议时间、时间格式化等场景
- **不适用于**耗时测量和超时控制（可能因系统调时而跳变）
- 格式化输出参考 `strftime`，默认格式为 ISO 8601
- `Now()` 静态方法内部调用 `std::chrono::system_clock::now()`

### 4.2 API

**获取当前时间：**

- `static WallTime Now()` - 获取当前墙上时钟时间

**工厂方法：**

- `static WallTime FromSeconds(int64_t sec)` - 从秒数构造
- `static WallTime FromMilliseconds(int64_t ms)` - 从毫秒数构造
- `static WallTime FromMicroseconds(int64_t us)` - 从微秒数构造
- `static WallTime FromNanoseconds(int64_t ns)` - 从纳秒数构造

**转换方法：**

- `int64_t ToSeconds() const` - 转换为秒数
- `int64_t ToMilliseconds() const` - 转换为毫秒数
- `int64_t ToMicroseconds() const` - 转换为微秒数
- `int64_t ToNanoseconds() const` - 转换为纳秒数

**格式化：**

- `std::string Format() const` - 默认格式（ISO 8601: `YYYY-MM-DDTHH:MM:SS.nnnnnnnnn`）
- `std::string Format(const std::string& fmt) const` - 自定义格式（strftime 语法）

**算术运算：**

- `WallTime operator+(const Duration& dur) const`
- `WallTime operator-(const Duration& dur) const`
- `Duration operator-(const WallTime& rhs) const`
- `WallTime& operator+=(const Duration& dur)`
- `WallTime& operator-=(const Duration& dur)`

**比较运算：**

- `bool operator==(const WallTime& rhs) const`
- `bool operator!=(const WallTime& rhs) const`
- `bool operator<(const WallTime& rhs) const`
- `bool operator<=(const WallTime& rhs) const`
- `bool operator>(const WallTime& rhs) const`
- `bool operator>=(const WallTime& rhs) const`

**std::chrono 互转：**

- `static WallTime FromChrono(const std::chrono::system_clock::time_point& tp)` - 从 chrono time\_point 构造
- `std::chrono::system_clock::time_point ToChrono() const` - 转换为 chrono time\_point（零开销）

### 4.3 使用示例

```cpp
using namespace com::etrita::eros::cytos::time;

auto now = WallTime::Now();
LOG(INFO) << now.ToMilliseconds();
LOG(INFO) << now.Format();  // 2025-07-01T12:30:45.123456789
LOG(INFO) << now.Format("%Y-%m-%d %H:%M:%S");  // 2025-07-01 12:30:45

auto one_hour_later = now + Duration::FromSeconds(3600);
LOG(INFO) << one_hour_later.Format();

// 快捷获取时间戳
LOG(INFO) << "Timestamp: " << WallTime::Now().ToMilliseconds();
```

***

## 5. MonoTime 单调时间

### 5.1 设计说明

`MonoTime` 表示单调时钟的时间点，内部基于 `std::chrono::steady_clock::time_point` 实现，保证单调递增，不受系统时间调整影响。

**实现说明：**

- 内部存储为 `std::chrono::steady_clock::time_point time_point_`
- 适用于耗时测量、超时控制、调度基准等场景
- **不适用于**表示真实世界时间
- 两个 MonoTime 的差值为 Duration
- `Now()` 静态方法内部调用 `std::chrono::steady_clock::now()`

### 5.2 API

**获取当前时间：**

- `static MonoTime Now()` - 获取当前单调时间

**算术运算：**

- `MonoTime operator+(const Duration& dur) const`
- `MonoTime operator-(const Duration& dur) const`
- `Duration operator-(const MonoTime& rhs) const`
- `MonoTime& operator+=(const Duration& dur)`
- `MonoTime& operator-=(const Duration& dur)`

**比较运算：**

- `bool operator==(const MonoTime& rhs) const`
- `bool operator!=(const MonoTime& rhs) const`
- `bool operator<(const MonoTime& rhs) const`
- `bool operator<=(const MonoTime& rhs) const`
- `bool operator>(const MonoTime& rhs) const`
- `bool operator>=(const MonoTime& rhs) const`

**std::chrono 互转：**

- `static MonoTime FromChrono(const std::chrono::steady_clock::time_point& tp)` - 从 chrono time\_point 构造
- `std::chrono::steady_clock::time_point ToChrono() const` - 转换为 chrono time\_point（零开销）

### 5.3 使用示例

```cpp
using namespace com::etrita::eros::cytos::time;

auto start = MonoTime::Now();
DoHeavyWork();
auto end = MonoTime::Now();

auto elapsed = end - start;
LOG(INFO) << "Elapsed: " << elapsed.ToMilliseconds() << "ms";
```

***

## 6. Clock 时钟抽象

### 6.1 设计说明

`Clock` 是时钟的抽象接口，提供获取当前时间的能力。通过依赖注入 Clock，业务代码可以在测试中使用 `FakeClock` 替代 `SystemClock`，实现时间 Mock。

**设计原则：**

- `Clock` 为纯虚接口，无状态
- `SystemClock` 为生产环境实现，内部使用 `std::chrono` 获取时间
- `FakeClock` 为测试环境实现，支持手动推进时间
- `WallTime::Now()` / `MonoTime::Now()` 是便捷 API，等价于 `SystemClock` 的调用

**为什么需要 Clock 抽象：**

- `WallTime::Now()` 和 `MonoTime::Now()` 直接调用系统时钟，无法在测试中控制时间
- 需要测试超时、定时等逻辑时，通过注入 `FakeClock` 替代系统时钟
- 业务类通过构造函数接收 `Clock*`，生产环境传入 `SystemClock`，测试环境传入 `FakeClock`

### 6.2 Clock 接口

**API：**

- `virtual ~Clock() = default`
- `virtual WallTime GetWallTime() = 0` - 获取当前墙上时钟时间
- `virtual MonoTime GetMonoTime() = 0` - 获取当前单调时间

### 6.3 SystemClock

生产环境时钟实现，基于 `std::chrono` 实现。

**API：**

- `WallTime GetWallTime() override` - 基于 `std::chrono::system_clock::now()`
- `MonoTime GetMonoTime() override` - 基于 `std::chrono::steady_clock::now()`

### 6.4 FakeClock

测试环境时钟实现，支持手动推进时间。

**实现说明：**

- 内部存储 `std::chrono::system_clock::time_point wall_time_` 和 `std::chrono::steady_clock::time_point mono_time_`
- `Advance` 通过增加 Duration 推进内部时间点

**API：**

- `FakeClock()` - 初始时间为 epoch
- `FakeClock(WallTime wall_time, MonoTime mono_time)` - 指定初始时间
- `WallTime GetWallTime() override`
- `MonoTime GetMonoTime() override`
- `void Advance(Duration duration)` - 同时推进 WallTime 和 MonoTime
- `void AdvanceWall(Duration duration)` - 仅推进 WallTime
- `void AdvanceMono(Duration duration)` - 仅推进 MonoTime
- `void SetWall(WallTime wall_time)` - 设置 WallTime
- `void SetMono(MonoTime mono_time)` - 设置 MonoTime

### 6.5 使用示例

**生产环境（使用静态方法）：**

```cpp
using namespace com::etrita::eros::cytos::time;

auto now = WallTime::Now();
auto mono = MonoTime::Now();
```

**测试环境（注入 FakeClock）：**

```cpp
class MyService {
 public:
  explicit MyService(Clock* clock) : clock_(clock), start_(clock_->GetMonoTime()) {}

  bool IsTimeout() {
    auto elapsed = clock_->GetMonoTime() - start_;
    return elapsed > Duration::FromSeconds(5);
  }

 private:
  Clock* clock_;
  MonoTime start_;
};

// 测试
FakeClock fake_clock;
MyService service(&fake_clock);

ASSERT_FALSE(service.IsTimeout());

// 推进时间
fake_clock.Advance(Duration::FromSeconds(6));

ASSERT_TRUE(service.IsTimeout());
```

***

## 7. Stopwatch 耗时统计

### 7.1 设计说明

`Stopwatch` 提供便捷的耗时统计功能，基于 MonoTime 实现。

**实现说明：**

- 内部使用 MonoTime 记录起始时间点
- 支持暂停/恢复，暂停期间不计入耗时
- 支持重置，重新开始计时
- 零动态分配
- 默认使用 `MonoTime::Now()`，支持注入 Clock 用于测试

### 7.2 API

**构造函数：**

- `Stopwatch()` - 构造时自动开始计时（使用 `MonoTime::Now()`）
- `explicit Stopwatch(Clock* clock)` - 注入 Clock（测试用）

**控制方法：**

- `void Reset()` - 重置计时器（重新开始）
- `void Pause()` - 暂停计时
- `void Resume()` - 恢复计时

**查询方法：**

- `Duration Elapsed() const` - 获取已耗时
- `int64_t ElapsedNanoseconds() const`
- `int64_t ElapsedMicroseconds() const`
- `int64_t ElapsedMilliseconds() const`
- `int64_t ElapsedSeconds() const`
- `bool IsPaused() const` - 是否暂停中

### 7.3 使用示例

```cpp
using namespace com::etrita::eros::cytos::time;

Stopwatch sw;
DoRpc();
LOG(INFO) << "RPC took " << sw.ElapsedMilliseconds() << "ms";

// 暂停/恢复
Stopwatch sw2;
sw2.Pause();
DoUnrelatedWork();  // 不计入耗时
sw2.Resume();
DoRelatedWork();
LOG(INFO) << "Related work took " << sw2.ElapsedMilliseconds() << "ms";

// 重置
sw.Reset();
DoAnotherTask();
LOG(INFO) << "Another task took " << sw.ElapsedMilliseconds() << "ms";
```

***

## 8. Deadline 超时控制

### 8.1 设计说明

`Deadline` 表示一个未来的截止时间点，用于超时控制场景。基于 MonoTime 实现，不受系统时间调整影响。

**实现说明：**

- 内部存储为 `MonoTime expire_time_`
- `After` 工厂方法基于当前 MonoTime 计算
- `IsExpired` 和 `Remaining` 基于当前 MonoTime 判断
- 支持注入 Clock，方便测试

### 8.2 API

**工厂方法：**

- `static Deadline After(Duration duration)` - 从当前时间开始计算截止时间（使用 `MonoTime::Now()`）
- `static Deadline After(Duration duration, Clock* clock)` - 注入 Clock

**查询方法：**

- `bool IsExpired() const` - 是否已超时
- `Duration Remaining() const` - 剩余时间（可能为负值，表示已超时多久）
- `MonoTime ExpireTime() const` - 获取截止时间点

### 8.3 使用示例

```cpp
using namespace com::etrita::eros::cytos::time;

auto deadline = Deadline::After(Duration::FromMilliseconds(500));

while (!deadline.IsExpired()) {
  if (TryConnect()) {
    break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

if (deadline.IsExpired()) {
  LOG(ERROR) << "Connection timeout";
}
```

***

## 9. 依赖

- C++20 标准库（`<chrono>`、`<thread>`）
- 无第三方依赖
- 无平台特定代码（由 `std::chrono` 处理平台差异）

***

## 10. 测试要求

### 10.1 Duration 测试

- 测试各单位的工厂方法和转换方法精度
- 测试算术运算（加、减、标量乘除、取反）
- 测试比较运算
- 测试零值和负值
- 测试溢出边界（极大/极小值）
- 测试 `Abs()` 方法
- 测试 `FromChrono()` / `ToChrono()` 与 `std::chrono::duration` 双向转换

### 10.2 WallTime 测试

- 测试 `Now()` 返回合理的时间值
- 测试 `FromSeconds/Milliseconds/Microseconds/Nanoseconds` 构造和 `ToSeconds/Milliseconds/Microseconds/Nanoseconds` 转换
- 测试格式化输出（默认格式和自定义格式）
- 测试算术运算（加减 Duration）
- 测试比较运算
- 测试 `FromChrono()` / `ToChrono()` 与 `std::chrono::system_clock::time_point` 双向转换

### 10.3 MonoTime 测试

- 测试 `Now()` 返回合理的时间值
- 测试构造和差值计算
- 测试算术运算（加减 Duration）
- 测试比较运算
- 测试 `FromChrono()` / `ToChrono()` 与 `std::chrono::steady_clock::time_point` 双向转换

### 10.4 Clock 测试

- 测试 `SystemClock` 返回合理的时间值
- 测试 `FakeClock` 时间推进
- 测试 `FakeClock` 独立推进 WallTime 和 MonoTime
- 测试通过 Clock 接口注入的依赖替换

### 10.5 Stopwatch 测试

- 测试基本计时功能
- 测试 `Reset()` 重置计时
- 测试 `Pause()`/`Resume()` 暂停恢复
- 测试注入 FakeClock 的计时

### 10.6 Deadline 测试

- 测试超时判断
- 测试剩余时间计算
- 测试注入 FakeClock 的超时控制

***

## 11. 性能要求

| 接口                            | 目标      | 说明                                |
| ----------------------------- | ------- | --------------------------------- |
| `MonoTime::Now()`             | < 100ns | 等价于 `steady_clock::now()`         |
| `WallTime::Now()`             | < 200ns | 等价于 `system_clock::now()`         |
| `Stopwatch`                   | 零动态分配   | 栈上 MonoTime + Duration            |
| `Duration` 算术                 | 内联优化    | 编译期委托给 `std::chrono::nanoseconds` |
| `ToChrono()` / `FromChrono()` | 零开销     | 直接返回/包装内部存储                       |

***

## 12. 文件结构

```
include/cytoskeleton/time/
├── duration.h           # Duration 时间间隔
├── wall_time.h          # WallTime 墙上时钟
├── mono_time.h          # MonoTime 单调时间
├── clock.h              # Clock 抽象接口
├── system_clock.h       # SystemClock 生产时钟
├── fake_clock.h         # FakeClock 测试时钟
├── stopwatch.h          # Stopwatch 耗时统计
└── deadline.h           # Deadline 超时控制

src/time/
├── duration.cpp
├── wall_time.cpp
├── mono_time.cpp
├── system_clock.cpp
├── fake_clock.cpp
├── stopwatch.cpp
└── deadline.cpp

tests/time/
├── duration_test.cpp
├── wall_time_test.cpp
├── mono_time_test.cpp
├── clock_test.cpp
├── stopwatch_test.cpp
└── deadline_test.cpp
```

***

## 13. 完整使用示例

```cpp
#include <cytoskeleton/time/duration.h>
#include <cytoskeleton/time/wall_time.h>
#include <cytoskeleton/time/mono_time.h>
#include <cytoskeleton/time/clock.h>
#include <cytoskeleton/time/fake_clock.h>
#include <cytoskeleton/time/stopwatch.h>
#include <cytoskeleton/time/deadline.h>

using namespace com::etrita::eros::cytos::time;

// ========== 获取当前时间 ==========
auto now = WallTime::Now();
LOG(INFO) << "Current time: " << now.Format();
LOG(INFO) << "Timestamp ms: " << now.ToMilliseconds();

auto mono = MonoTime::Now();

// ========== 统计耗时 ==========
Stopwatch sw;
DoHeavyWork();
LOG(INFO) << "Elapsed: " << sw.ElapsedMilliseconds() << "ms";

// ========== 超时控制 ==========
auto deadline = Deadline::After(Duration::FromMilliseconds(500));
while (!deadline.IsExpired()) {
  if (TryConnect()) {
    break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// ========== 测试中使用 FakeClock ==========
class TimerService {
 public:
  explicit TimerService(Clock* clock) : clock_(clock), start_(clock_->GetMonoTime()) {}

  bool WaitForEvent() {
    auto deadline = Deadline::After(Duration::FromSeconds(5), clock_);
    while (!deadline.IsExpired()) {
      if (CheckEvent()) return true;
    }
    return false;
  }

 private:
  Clock* clock_;
  MonoTime start_;
};

// 测试代码
FakeClock fake_clock;
TimerService service(&fake_clock);

ASSERT_FALSE(service.WaitForEvent());

// 推进时间模拟超时
fake_clock.Advance(Duration::FromSeconds(6));
ASSERT_TRUE(service.WaitForEvent() == false);  // 已超时

// ========== Duration 算术 ==========
auto timeout = Duration::FromMilliseconds(500);
auto retry_interval = Duration::FromMilliseconds(50);
int max_retries = timeout / retry_interval;  // 10

// ========== 与 std::chrono 互操作（零开销） ==========
auto chrono_dur = std::chrono::seconds(3);
auto my_dur = Duration::FromChrono(chrono_dur);
LOG(INFO) << my_dur.ToMilliseconds();  // 3000

auto wall_tp = WallTime::Now().ToChrono();  // 直接获取 system_clock::time_point
auto mono_tp = MonoTime::Now().ToChrono();  // 直接获取 steady_clock::time_point
```

