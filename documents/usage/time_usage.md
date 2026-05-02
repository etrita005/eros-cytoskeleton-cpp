# Time 模块使用文档

## 1. 概述

Time 模块提供统一、易用的时间基础设施，基于 `std::chrono` 实现，提供更直观、更安全的 API 包装。

### 1.1 命名空间

```cpp
using namespace com::etrita::eros::cytos::time;
```

### 1.2 头文件

```cpp
#include "cytoskeleton/time/time.h"  // 包含所有 Time 模块头文件
```

也可按需包含单个头文件：

```cpp
#include "cytoskeleton/time/duration.h"
#include "cytoskeleton/time/wall_time.h"
#include "cytoskeleton/time/mono_time.h"
#include "cytoskeleton/time/clock.h"
#include "cytoskeleton/time/stopwatch.h"
#include "cytoskeleton/time/deadline.h"
```

### 1.3 Bazel 依赖

```python
deps = ["//include/cytoskeleton/time:time"]
```

### 1.4 设计原则

- **安全性 > 易用性 > 性能**
- 基于 `std::chrono` 实现，零开销包装
- 纯头文件库，无需编译链接
- 明确区分 WallTime（墙上时钟）和 MonoTime（单调时钟）

***

## 2. 核心类型

### 2.1 类型总览

| 类型         | 含义     | 内部存储                       | 典型用途      |
| ---------- | ------ | -------------------------- | --------- |
| `Duration` | 时间间隔   | `std::chrono::nanoseconds` | 超时参数、延迟   |
| `WallTime` | 真实世界时间 | `system_clock::time_point` | 日志时间戳、格式化 |
| `MonoTime` | 单调时间点  | `steady_clock::time_point` | 耗时测量、超时控制 |

***

## 3. Duration 时间间隔

### 3.1 创建 Duration

```cpp
// 从不同单位创建
auto d1 = Duration::FromNanoseconds(100);
auto d2 = Duration::FromMicroseconds(500);
auto d3 = Duration::FromMilliseconds(1000);
auto d4 = Duration::FromSeconds(5);
auto d5 = Duration::FromHours(2);
auto d6 = Duration::FromSecondsDouble(1.5);  // 浮点秒

// 零值
auto zero = Duration::Zero();
```

### 3.2 转换

```cpp
auto d = Duration::FromMilliseconds(1500);

d.ToNanoseconds();    // 1500000000
d.ToMicroseconds();   // 1500000
d.ToMilliseconds();   // 1500
d.ToSeconds();        // 1
d.ToSecondsDouble();  // 1.5
```

### 3.3 算术运算

```cpp
auto a = Duration::FromMilliseconds(500);
auto b = Duration::FromSeconds(1);

auto sum = a + b;          // 1500ms
auto diff = b - a;         // 500ms
auto doubled = a * 2;      // 1000ms
auto halved = b / 2;       // 500ms
auto ratio = b / a;        // 2 (倍数关系)
auto neg = -a;             // -500ms

a += Duration::FromMilliseconds(100);  // a = 600ms
a -= Duration::FromMilliseconds(100);  // a = 500ms
```

### 3.4 比较运算

```cpp
auto a = Duration::FromMilliseconds(100);
auto b = Duration::FromMilliseconds(200);

a == b;  // false
a != b;  // true
a < b;   // true
a <= b;  // true
a > b;   // false
a >= b;  // false
```

### 3.5 工具方法

```cpp
auto d = Duration::FromMilliseconds(-100);

d.IsZero();       // false
d.IsNegative();   // true
d.Abs();          // 100ms
```

### 3.6 与 std::chrono 互转

```cpp
// Duration -> std::chrono
auto d = Duration::FromMilliseconds(100);
auto chrono_ns = d.ToChrono();  // std::chrono::nanoseconds

// std::chrono -> Duration
auto chrono_dur = std::chrono::seconds(5);
auto my_dur = Duration::FromChrono(chrono_dur);
```

***

## 4. WallTime 墙上时钟

### 4.1 获取当前时间

```cpp
auto now = WallTime::Now();
```

### 4.2 从时间戳构造

```cpp
auto t1 = WallTime::FromSeconds(1700000000);
auto t2 = WallTime::FromMilliseconds(1700000000000LL);
auto t3 = WallTime::FromMicroseconds(1700000000000000LL);
auto t4 = WallTime::FromNanoseconds(1700000000000000000LL);
```

### 4.3 转换为时间戳

```cpp
auto now = WallTime::Now();
now.ToSeconds();        // Unix 秒
now.ToMilliseconds();   // Unix 毫秒
now.ToMicroseconds();   // Unix 微秒
now.ToNanoseconds();    // Unix 纳秒
```

### 4.4 格式化

```cpp
auto now = WallTime::Now();

// 默认格式 (ISO 8601)
now.Format();  // "2025-07-01T12:30:45.123456789"

// 自定义格式 (strftime 语法)
now.Format("%Y-%m-%d %H:%M:%S");  // "2025-07-01 12:30:45"
now.Format("%Y/%m/%d");            // "2025/07/01"
```

### 4.5 时间偏移

```cpp
auto now = WallTime::Now();
auto one_hour_later = now + Duration::FromHours(1);
auto one_hour_ago = now - Duration::FromHours(1);

// 两个 WallTime 的差值
auto diff = one_hour_later - now;  // Duration(3600s)
```

### 4.6 与 std::chrono 互转

```cpp
// WallTime -> std::chrono
auto now = WallTime::Now();
auto tp = now.ToChrono();  // system_clock::time_point

// std::chrono -> WallTime
auto wall = WallTime::FromChrono(tp);
```

***

## 5. MonoTime 单调时间

### 5.1 获取当前时间

```cpp
auto mono = MonoTime::Now();
```

### 5.2 耗时测量

```cpp
auto start = MonoTime::Now();
DoHeavyWork();
auto end = MonoTime::Now();

auto elapsed = end - start;  // Duration
LOG(INFO) << "耗时: " << elapsed.ToMilliseconds() << "ms";
```

### 5.3 时间偏移

```cpp
auto now = MonoTime::Now();
auto later = now + Duration::FromSeconds(10);
auto earlier = now - Duration::FromSeconds(5);
```

### 5.4 与 std::chrono 互转

```cpp
auto mono = MonoTime::Now();
auto tp = mono.ToChrono();  // steady_clock::time_point
auto restored = MonoTime::FromChrono(tp);
```

***

## 6. Clock 时钟抽象

### 6.1 为什么需要 Clock

`WallTime::Now()` 和 `MonoTime::Now()` 直接调用系统时钟，在测试中无法控制时间。通过注入 `Clock` 接口，可以在测试中使用 `FakeClock` 替代。

### 6.2 Clock 接口

```cpp
class Clock {
 public:
  virtual ~Clock() = default;
  virtual WallTime GetWallTime() = 0;
  virtual MonoTime GetMonoTime() = 0;
};
```

### 6.3 SystemClock

生产环境使用，直接调用系统时钟：

```cpp
SystemClock clock;
auto wall = clock.GetWallTime();  // 等价于 WallTime::Now()
auto mono = clock.GetMonoTime();  // 等价于 MonoTime::Now()
```

### 6.4 FakeClock

测试环境使用，支持手动推进时间：

```cpp
// 创建 FakeClock（初始时间为 epoch）
FakeClock fake_clock;

// 同时推进 Wall 和 Mono
fake_clock.Advance(Duration::FromSeconds(10));

// 仅推进 Wall
fake_clock.AdvanceWall(Duration::FromHours(1));

// 仅推进 Mono
fake_clock.AdvanceMono(Duration::FromMilliseconds(100));

// 直接设置时间
fake_clock.SetWall(WallTime::FromSeconds(1700000000));
fake_clock.SetMono(MonoTime::Now());

// 带初始时间构造
FakeClock fake_clock2(WallTime::FromSeconds(1700000000), MonoTime::Now());
```

### 6.5 依赖注入模式

```cpp
class MyService {
 public:
  explicit MyService(Clock* clock) : clock_(clock) {}

  bool WaitForEvent(Duration timeout) {
    auto deadline = Deadline::After(timeout, clock_);
    while (!deadline.IsExpired()) {
      if (CheckEvent()) return true;
    }
    return false;
  }

 private:
  Clock* clock_;
};

// 生产环境
SystemClock system_clock;
MyService service(&system_clock);

// 测试环境
FakeClock fake_clock;
MyService test_service(&fake_clock);
```

***

## 7. Stopwatch 耗时统计

### 7.1 基本用法

```cpp
Stopwatch sw;
DoHeavyWork();
LOG(INFO) << "耗时: " << sw.ElapsedMilliseconds() << "ms";
```

### 7.2 获取耗时

```cpp
Stopwatch sw;
// ...

sw.Elapsed();              // Duration 对象
sw.ElapsedNanoseconds();   // int64_t
sw.ElapsedMicroseconds();  // int64_t
sw.ElapsedMilliseconds();  // int64_t
sw.ElapsedSeconds();       // int64_t
```

### 7.3 暂停与恢复

```cpp
Stopwatch sw;
DoRelatedWork();
sw.Pause();
DoUnrelatedWork();  // 不计入耗时
sw.Resume();
DoMoreRelatedWork();
LOG(INFO) << "相关耗时: " << sw.ElapsedMilliseconds() << "ms";
```

### 7.4 重置

```cpp
Stopwatch sw;
DoFirstTask();
LOG(INFO) << "任务1: " << sw.ElapsedMilliseconds() << "ms";

sw.Reset();
DoSecondTask();
LOG(INFO) << "任务2: " << sw.ElapsedMilliseconds() << "ms";
```

### 7.5 注入 Clock（测试用）

```cpp
FakeClock fake_clock;
Stopwatch sw(&fake_clock);

// 推进时间
fake_clock.Advance(Duration::FromMilliseconds(100));
EXPECT_EQ(sw.ElapsedMilliseconds(), 100);
```

***

## 8. Deadline 超时控制

### 8.1 基本用法

```cpp
auto deadline = Deadline::After(Duration::FromMilliseconds(500));

while (!deadline.IsExpired()) {
  if (TryConnect()) {
    break;
  }
}

if (deadline.IsExpired()) {
  LOG(ERROR) << "连接超时";
}
```

### 8.2 查询剩余时间

```cpp
auto deadline = Deadline::After(Duration::FromSeconds(10));

auto remaining = deadline.Remaining();
if (remaining.IsNegative()) {
  LOG(INFO) << "已超时 " << remaining.Abs().ToMilliseconds() << "ms";
} else {
  LOG(INFO) << "剩余 " << remaining.ToMilliseconds() << "ms";
}
```

### 8.3 获取截止时间点

```cpp
auto deadline = Deadline::After(Duration::FromSeconds(5));
auto expire_time = deadline.ExpireTime();  // MonoTime
```

### 8.4 注入 Clock（测试用）

```cpp
FakeClock fake_clock;
auto deadline = Deadline::After(Duration::FromSeconds(5), &fake_clock);

EXPECT_FALSE(deadline.IsExpired());

// 推进时间模拟超时
fake_clock.Advance(Duration::FromSeconds(6));
EXPECT_TRUE(deadline.IsExpired());
```

***

## 9. WallTime vs MonoTime 选择指南

| 场景     | 使用                | 原因             |
| ------ | ----------------- | -------------- |
| 日志时间戳  | `WallTime::Now()` | 需要人类可读时间       |
| 协议时间字段 | `WallTime::Now()` | 需要与外部系统对齐      |
| 耗时测量   | `MonoTime::Now()` | 不受系统调时影响       |
| 超时控制   | `MonoTime::Now()` | 保证单调递增         |
| 定时调度   | `MonoTime::Now()` | 避免时间跳变导致调度异常   |
| 时间格式化  | `WallTime`        | MonoTime 无法格式化 |

**核心原则：需要"时刻"用 WallTime，需要"间隔"用 MonoTime。**

***

## 10. 与 std::chrono 的关系

Time 模块基于 `std::chrono` 实现，所有类型均可零开销地与 `std::chrono` 互转：

```cpp
// Duration <-> std::chrono::nanoseconds
auto d = Duration::FromMilliseconds(100);
auto chrono_ns = d.ToChrono();  // 零开销

// WallTime <-> system_clock::time_point
auto wall = WallTime::Now();
auto sys_tp = wall.ToChrono();  // 零开销

// MonoTime <-> steady_clock::time_point
auto mono = MonoTime::Now();
auto steady_tp = mono.ToChrono();  // 零开销

// 与第三方库交互
void ThirdPartyFunc(std::chrono::milliseconds timeout);
auto my_timeout = Duration::FromSeconds(2);
ThirdPartyFunc(std::chrono::duration_cast<std::chrono::milliseconds>(my_timeout.ToChrono()));
```

***

## 11. 完整示例

### 11.1 带超时的重试逻辑

```cpp
#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

bool ConnectWithRetry(const std::string& address, Duration timeout) {
  auto deadline = Deadline::After(timeout);
  int attempt = 0;

  while (!deadline.IsExpired()) {
    ++attempt;
    if (TryConnect(address)) {
      LOG(INFO) << "连接成功, 尝试次数: " << attempt;
      return true;
    }
    LOG(INFO) << "尝试 " << attempt << " 失败, 剩余: "
              << deadline.Remaining().ToMilliseconds() << "ms";
  }

  LOG(ERROR) << "连接超时";
  return false;
}

// 使用
ConnectWithRetry("192.168.1.1:8080", Duration::FromSeconds(10));
```

### 11.2 可测试的定时服务

```cpp
#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

class PeriodicChecker {
 public:
  explicit PeriodicChecker(Clock* clock, Duration interval)
      : clock_(clock), interval_(interval), last_check_(clock_->GetMonoTime()) {}

  bool ShouldCheck() const {
    auto elapsed = clock_->GetMonoTime() - last_check_;
    return elapsed >= interval_;
  }

  void MarkChecked() {
    last_check_ = clock_->GetMonoTime();
  }

 private:
  Clock* clock_;
  Duration interval_;
  MonoTime last_check_;
};

// 生产环境
SystemClock system_clock;
PeriodicChecker checker(&system_clock, Duration::FromSeconds(30));

// 测试环境
FakeClock fake_clock;
PeriodicChecker test_checker(&fake_clock, Duration::FromSeconds(30));
EXPECT_FALSE(test_checker.ShouldCheck());
fake_clock.Advance(Duration::FromSeconds(31));
EXPECT_TRUE(test_checker.ShouldCheck());
```

### 11.3 性能计时器

```cpp
#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

class ScopedTimer {
 public:
  explicit ScopedTimer(const std::string& name)
      : name_(name), sw_() {}

  ~ScopedTimer() {
    LOG(INFO) << name_ << ": " << sw_.ElapsedMilliseconds() << "ms";
  }

  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;

 private:
  std::string name_;
  Stopwatch sw_;
};

// 使用
void ProcessData() {
  ScopedTimer timer("ProcessData");
  // ... 处理数据
}
```

