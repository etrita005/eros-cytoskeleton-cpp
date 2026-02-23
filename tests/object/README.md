# Object 模块单元测试文档

## 概述

本文档描述了 Cytoskeleton Object 模块的单元测试用例，包括测试目标、测试方法和预期结果。

## 测试环境

- **测试框架**: Google Test (gtest)
- **构建系统**: Bazel 9
- **C++ 标准**: C++20
- **命名空间**: `com::etrita::eros::cytos::object`

## 测试文件说明

### 1. object_test.cpp

测试 Object 基类的核心功能。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `GetSharedPtrReturnsValidPointer` | 验证 GetSharedPtr() 返回有效的 shared_ptr | 返回的指针不为空，且指向同一对象 |
| `GetSharedPtrConstReturnsValidPointer` | 验证 const 版本的 GetSharedPtr() | 返回的 const 指针有效 |
| `LockUnlockProtectsData` | 验证 Lock/Unlock 的线程安全性 | 两个线程各递增1000次，最终值为2000 |
| `TryLockSucceedsWhenUnlocked` | 验证 TryLock 在未加锁时成功 | 返回 true，值递增 |
| `NotifyAndJoin` | 验证 Notify/Join 基本功能 | Join 在 Notify 后成功返回 |
| `JoinWithTimeoutReturnsTrueWhenNotified` | 验证带超时的 Join 在通知时返回 true | 在超时前收到通知，返回 true |
| `JoinWithTimeoutReturnsFalseWhenTimeout` | 验证带超时的 Join 在超时时返回 false | 未收到通知，返回 false |
| `ResetNotifyClearsSignal` | 验证 ResetNotify 清除信号 | 通知后被重置，Join 超时返回 false |
| `AutoResetAfterJoin` | 验证 Join 后自动重置 | Join 成功后再次 Join 会超时 |
| `GetMutexReturnsValidMutex` | 验证 GetMutex 返回有效的互斥锁 | 返回的 mutex 可正常加锁 |

### 2. singleton_test.cpp

测试 Singleton 单例模板。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `InstanceReturnsSameObject` | 验证多次调用 Instance 返回同一对象 | 所有调用返回的指针相同 |
| `InstanceReturnsValidObject` | 验证 Instance 返回有效对象 | 对象不为空，成员值正确 |
| `DifferentSingletonsAreIndependent` | 验证不同单例类相互独立 | 不同单例的实例不同 |
| `ThreadSafeInitialization` | 验证多线程环境下线程安全初始化 | 所有线程获得同一实例 |
| `SharedPtrReferenceCounting` | 验证 shared_ptr 引用计数 | 实例在引用存在时不被销毁 |

### 3. lifecycled_object_test.cpp

测试 LifecycledObject 生命周期管理。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `InitialStateIsUninitialized` | 验证初始状态为 Uninitialized | IsUninitialized() 返回 true |
| `InitializeTransitionsToInitialized` | 验证 Initialize 状态转换 | 状态变为 Initialized，回调被调用 |
| `InitializeIsIdempotent` | 验证 Initialize 幂等性 | 多次调用只执行一次回调 |
| `InitializeFailureKeepsUninitialized` | 验证 Initialize 失败保持原状态 | 状态保持 Uninitialized |
| `StartTransitionsToRunning` | 验证 Start 状态转换 | 状态变为 Running |
| `StartBeforeInitializeFails` | 验证未初始化时 Start 失败 | Start 返回 false |
| `StartIsIdempotentWhenRunning` | 验证 Start 幂等性 | 多次调用只执行一次回调 |
| `StartFailureTransitionsToStopped` | 验证 Start 失败转到 Stopped | 状态变为 Stopped |
| `StopTransitionsToStopped` | 验证 Stop 状态转换 | 状态变为 Stopped |
| `StopWhenNotRunningIsIdempotent` | 验证 Stop 幂等性 | 未运行时调用不执行回调 |
| `DestroyTransitionsToDestroyed` | 验证 Destroy 状态转换 | 状态变为 Destroyed |
| `DestroyIsIdempotent` | 验证 Destroy 幂等性 | 多次调用只执行一次回调 |
| `DestroyStopsRunningObject` | 验证 Destroy 自动停止运行中对象 | 调用 Stop 回调后销毁 |
| `RestartAfterStop` | 验证停止后可重新启动 | 可再次 Start 并进入 Running |
| `DestructorCallsDestroy` | 验证析构时自动调用 Destroy | 析构时执行销毁回调 |
| `StateTransitions` | 验证完整状态转换流程 | 状态按预期顺序转换 |
| `ThreadSafeStateTransitions` | 验证状态转换线程安全 | 多线程调用只执行一次回调 |

### 4. auto_start_lifecycled_object_test.cpp

测试 AutoStartLifecycledObject 自启动生命周期对象。

#### 测试用例列表

| 测试用例 | 描述 | 预期结果 |
|---------|------|---------|
| `CreateReturnsSharedPtr` | 验证 Create 工厂方法 | 返回有效的 shared_ptr |
| `CreateWithLambdaReturnsSharedPtr` | 验证 CreateWithLambda 工厂方法 | 返回有效的 shared_ptr |
| `StartStartsThread` | 验证 Start 启动线程 | Run 方法被执行 |
| `StopStopsThread` | 验证 Stop 停止线程 | 线程停止执行 |
| `RestartAfterStop` | 验证停止后可重新启动 | 线程可再次启动执行 |
| `DestructorStopsThread` | 验证析构时自动停止线程 | 析构时线程已停止 |
| `LambdaFunctionExecutes` | 验证 Lambda 构造方式 | Lambda 函数被执行 |
| `StopTokenWorks` | 验证 stop_token 功能 | Stop 后 stop_requested 返回 true |
| `DestroyStopsThread` | 验证 Destroy 停止线程 | 线程在 Destroy 后停止 |
| `ThreadSafeStartStop` | 验证 Start/Stop 线程安全 | 多线程调用不会崩溃 |
| `OnStartOnStopCallbacksCalled` | 验证生命周期回调 | OnStart/OnStop 被正确调用 |

## 运行测试

### 运行所有测试

```bash
bazel test //tests/object/...
```

### 运行单个测试文件

```bash
bazel test //tests/object:object_test
bazel test //tests/object:singleton_test
bazel test //tests/object:lifecycled_object_test
bazel test //tests/object:auto_start_lifecycled_object_test
```

### 运行特定测试用例

```bash
bazel test //tests/object:object_test --test_filter=ObjectTest.GetSharedPtrReturnsValidPointer
```

### 查看详细输出

```bash
bazel test //tests/object:object_test --test_output=all
```

## 测试覆盖范围

- **Object 基类**: 通知机制、互斥锁、shared_ptr 管理
- **Singleton 模板**: 单例唯一性、线程安全初始化
- **LifecycledObject**: 9 状态生命周期管理、幂等性、线程安全
- **AutoStartLifecycledObject**: 自动线程管理、工厂方法、Lambda 支持

## 注意事项

1. 所有测试使用 `std::make_shared` 创建对象，确保与 `shared_from_this` 兼容
2. 多线程测试使用 `std::thread` 和 `std::atomic` 确保线程安全
3. 超时测试使用 `std::chrono::milliseconds` 控制时间
4. 测试完成后自动清理资源，避免内存泄漏
