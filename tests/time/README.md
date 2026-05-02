# Time 模块单元测试文档

## 概述

本文档描述了 Cytoskeleton Time 模块的单元测试用例，包括测试目标、测试方法和预期结果。

## 测试环境

- **测试框架**: Google Test (gtest)
- **构建系统**: Bazel 9
- **C++ 标准**: C++20
- **命名空间**: `com::etrita::eros::cytos::time`

## 测试文件说明

### 1. duration_test.cpp

测试 Duration 时间间隔类的核心功能。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `ZeroIsZero` | 验证 Zero() 返回零值 | IsZero() 返回 true，纳秒值为 0 |
| `FromNanoseconds` | 验证从纳秒构造 | 纳秒值正确 |
| `FromMicroseconds` | 验证从微秒构造 | 微秒和纳秒值正确 |
| `FromMilliseconds` | 验证从毫秒构造 | 毫秒和微秒值正确 |
| `FromSeconds` | 验证从秒构造 | 秒和毫秒值正确 |
| `FromSecondsDouble` | 验证从浮点秒构造 | 毫秒值正确 |
| `ToSecondsDouble` | 验证转换为浮点秒 | 值接近 1.5 |
| `Addition` | 验证加法运算 | 500ms + 1s = 1500ms |
| `Subtraction` | 验证减法运算 | 1s - 500ms = 500ms |
| `ScalarMultiplication` | 验证标量乘法 | 500ms * 2 = 1000ms |
| `ScalarDivision` | 验证标量除法 | 1000ms / 2 = 500ms |
| `DurationDivision` | 验证 Duration 除法 | 500ms / 50ms = 10 |
| `Negation` | 验证取反运算 | 100ms 取反为 -100ms |
| `CompoundAddition` | 验证复合加法 | 100ms += 200ms = 300ms |
| `CompoundSubtraction` | 验证复合减法 | 300ms -= 100ms = 200ms |
| `Equality` | 验证相等比较 | 相同值相等，不同值不等 |
| `Comparison` | 验证大小比较 | 100ms < 200ms |
| `IsNegative` | 验证负值判断 | -100ms 为负，0 和正值不为负 |
| `Abs` | 验证绝对值 | -100ms.Abs() = 100ms |
| `FromChrono` | 验证从 chrono 构造 | 5s 转换正确 |
| `ToChrono` | 验证转换为 chrono | 100ms 转换正确 |
| `ChronoRoundTrip` | 验证 chrono 双向转换 | 往返转换值不变 |

### 2. wall_time_test.cpp

测试 WallTime 墙上时钟时间类。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `NowReturnsValidTime` | 验证 Now() 返回有效时间 | 秒数和毫秒数大于 0 |
| `FromSeconds` | 验证从秒数构造 | 秒数正确 |
| `FromMilliseconds` | 验证从毫秒数构造 | 毫秒数正确 |
| `FromMicroseconds` | 验证从微秒数构造 | 微秒数正确 |
| `FromNanoseconds` | 验证从纳秒数构造 | 纳秒数正确 |
| `FormatDefault` | 验证默认格式化 | 包含 ISO 8601 格式的 T |
| `FormatCustom` | 验证自定义格式化 | 格式符合预期 |
| `Addition` | 验证加 Duration | 时间点正确偏移 |
| `Subtraction` | 验证两个 WallTime 相减 | 差值为正确 Duration |
| `DurationSubtraction` | 验证减 Duration | 时间点正确偏移 |
| `CompoundAddition` | 验证复合加法 | 时间点正确偏移 |
| `CompoundSubtraction` | 验证复合减法 | 时间点正确偏移 |
| `Equality` | 验证相等比较 | 相同值相等 |
| `Comparison` | 验证大小比较 | 较小值 < 较大值 |
| `FromChrono` | 验证从 chrono 构造 | 秒数正确 |
| `ToChrono` | 验证转换为 chrono | 秒数正确 |
| `ChronoRoundTrip` | 验证 chrono 双向转换 | 往返转换值不变 |

### 3. mono_time_test.cpp

测试 MonoTime 单调时间类。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `NowReturnsValidTime` | 验证 Now() 返回有效值 | 无异常 |
| `NowIsMonotonicallyIncreasing` | 验证单调递增 | 后调用 >= 前调用 |
| `SubtractionGivesDuration` | 验证差值为 Duration | sleep 50ms 后差值约 50ms |
| `Addition` | 验证加 Duration | 差值正确 |
| `SubtractionWithDuration` | 验证减 Duration | 差值正确 |
| `CompoundAddition` | 验证复合加法 | 差值正确 |
| `CompoundSubtraction` | 验证复合减法 | 差值正确 |
| `Equality` | 验证相等比较 | 相同值相等 |
| `Comparison` | 验证大小比较 | 较小值 < 较大值 |
| `FromChrono` | 验证从 chrono 构造 | 无异常 |
| `ToChrono` | 验证转换为 chrono | 往返值相等 |
| `ChronoRoundTrip` | 验证 chrono 双向转换 | 往返值相等 |

### 4. clock_test.cpp

测试 Clock 抽象接口和 SystemClock/FakeClock 实现。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `SystemClockReturnsValidTime` | 验证 SystemClock 返回有效时间 | 时间值大于 0 |
| `SystemClockWallTimeMatchesNow` | 验证 SystemClock 与 WallTime::Now() 一致 | 差值小于 2 秒 |
| `FakeClockInitialTimeIsEpoch` | 验证 FakeClock 初始时间为 epoch | 纳秒值为 0 |
| `FakeClockAdvanceWallAndMono` | 验证同时推进 Wall 和 Mono | 两者都推进 10 秒 |
| `FakeClockAdvanceWallOnly` | 验证仅推进 Wall | Wall 推进，Mono 不变 |
| `FakeClockAdvanceMonoOnly` | 验证仅推进 Mono | Mono 推进，Wall 不变 |
| `FakeClockSetWall` | 验证设置 WallTime | WallTime 为设定值 |
| `FakeClockSetMono` | 验证设置 MonoTime | MonoTime 差值正确 |
| `FakeClockConstructedWithTime` | 验证带初始时间构造 | WallTime 为设定值 |
| `ClockPolymorphism` | 验证 Clock 多态使用 | 通过基类指针调用正确 |

### 5. stopwatch_test.cpp

测试 Stopwatch 耗时统计工具。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `BasicTiming` | 验证基本计时功能 | sleep 50ms 后耗时约 50ms |
| `ElapsedMilliseconds` | 验证毫秒级耗时 | sleep 50ms 后约 50ms |
| `ElapsedSeconds` | 验证初始秒数为 0 | 返回 0 |
| `Reset` | 验证重置计时器 | 重置后耗时接近 0 |
| `PauseAndResume` | 验证暂停/恢复 | 暂停期间不计入耗时 |
| `IsPausedInitially` | 验证初始未暂停 | 返回 false |
| `WithFakeClock` | 验证注入 FakeClock | 推进时间后耗时正确 |
| `PauseWithFakeClock` | 验证 FakeClock 下暂停 | 暂停期间不计入 |
| `ResetWithFakeClock` | 验证 FakeClock 下重置 | 重置后耗时从 0 开始 |

### 6. deadline_test.cpp

测试 Deadline 超时控制工具。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `NotExpiredInitially` | 验证初始未超时 | IsExpired() 返回 false |
| `ExpiredAfterDuration` | 验证超时后过期 | sleep 后 IsExpired() 返回 true |
| `RemainingIsPositive` | 验证剩余时间为正 | 大于 0 |
| `RemainingIsNegativeAfterExpiry` | 验证超时后剩余为负 | IsNegative() 返回 true |
| `ExpireTime` | 验证截止时间点 | 在预期范围内 |
| `WithFakeClock` | 验证注入 FakeClock | 推进时间后超时判断正确 |
| `WithFakeClockExactExpiry` | 验证精确超时 | 推进恰好到时间后过期 |
| `ZeroDeadlineIsExpired` | 验证零延迟立即过期 | IsExpired() 返回 true |

## 运行测试

### 运行所有测试

```bash
bazel test //tests/time/...
```

### 运行单个测试文件

```bash
bazel test //tests/time:duration_test
bazel test //tests/time:wall_time_test
bazel test //tests/time:mono_time_test
bazel test //tests/time:clock_test
bazel test //tests/time:stopwatch_test
bazel test //tests/time:deadline_test
```

### 运行特定测试用例

```bash
bazel test //tests/time:duration_test --test_filter=DurationTest.Addition
```

### 查看详细输出

```bash
bazel test //tests/time:duration_test --test_output=all
```

## 测试覆盖范围

- **Duration**: 工厂方法、转换方法、算术运算、比较运算、零值/负值/绝对值、chrono 互转
- **WallTime**: Now()、工厂方法、格式化、算术运算、比较运算、chrono 互转
- **MonoTime**: Now()、单调性、差值计算、算术运算、比较运算、chrono 互转
- **Clock**: SystemClock、FakeClock 推进/设置、多态使用
- **Stopwatch**: 基本计时、暂停/恢复、重置、FakeClock 注入
- **Deadline**: 超时判断、剩余时间、FakeClock 注入、零延迟

## 注意事项

1. 时间相关测试使用宽松的范围检查，避免因系统调度延迟导致测试不稳定
2. FakeClock 测试不依赖真实时间流逝，确保测试确定性和快速执行
3. Duration 基于 `std::chrono::nanoseconds` 实现，与 chrono 的互转为零开销
