# Cytoskeleton C++ 基础库

Cytoskeleton 是一个面向机器人应用与服务开发的 C++20 基础库，提供并发控制、对象生命周期管理、消息队列、数据结构等核心功能。

## 设计哲学

### 核心原则

1. **安全性 > 易用性 > 性能**
   - 面向机器人应用，可靠性优先
   - 不面向高性能计算场景

2. **Header-Only 设计**
   - 大部分模块的实现都在头文件中
   - 便于集成和部署
   - **例外**：`module` 模块由于需要维护全局状态（动态库注册信息），需要编译为静态库或动态库

3. **现代 C++20**
   - 充分利用 `std::jthread`、`std::stop_token`、concepts 等特性
   - 遵循 Google C++ Style Guide

4. **模块化架构**
   - 各模块独立，可单独使用
   - 清晰的依赖关系

## 模块概览

| 模块 | 说明 | Header-Only | 状态 |
|------|------|-------------|------|
| [concurrent](include/cytoskeleton/concurrent/README.md) | 并发控制：线程、锁、事件、线程安全容器 | ✅ 是 | ✅ 已完成 |
| [object](include/cytoskeleton/object/README.md) | 对象基础：生命周期管理、单例模式 | ✅ 是 | ✅ 已完成 |
| [itc/message_queue](include/cytoskeleton/itc/message_queue/) | 消息队列：基于消息的异步通信机制 | ✅ 是 | ✅ 已完成 |
| [module](include/cytoskeleton/module/README.md) | 动态模块加载与管理：插件化架构、动态库加载 | ❌ 否 | ✅ 已完成 |

**注意：** `module` 模块由于需要维护全局状态（动态库注册信息），不能实现为 header-only 库，需要编译为静态库或动态库。

## 快速开始

### 环境要求

- C++20 兼容编译器 (GCC 11+, Clang 14+, MSVC 2022+)
- Bazel 7.0+

### Bazel 集成

```python
# MODULE.bazel
bazel_dep(name = "cytoskeleton", version = "0.1.0")

# BUILD.bazel
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = [
        "@cytoskeleton//include/cytoskeleton/concurrent",
        "@cytoskeleton//include/cytoskeleton/object",
        "@cytoskeleton//include/cytoskeleton/itc/message_queue",
        # 注意：module 模块需要编译链接，不是 header-only
        "@cytoskeleton//cytoskeleton/module",
    ],
)
```

### 编译项目

```bash
# 编译所有目标
bazel build //:all

# 编译特定模块
bazel build //include/cytoskeleton/concurrent:all
bazel build //include/cytoskeleton/object:all
bazel build //include/cytoskeleton/itc/message_queue:all
bazel build //include/cytoskeleton/module:all

# 编译示例程序
bazel build //examples/...

# 编译测试程序
bazel build //tests/...
```

### 交叉编译

项目支持 ARM64 交叉编译和原生编译，详细文档请参考 [EROS Forge 构建系统](../../forge/README.md)。

```bash
# 交叉编译（x86_64 主机 -> ARM64 目标）
bazel build --config=cross_arm64 //:all

# ARM64 原生编译（在 ARM64 主机上）
bazel build --config=arm64 //:all

# x86_64 原生编译（在 x86_64 主机上）
bazel build --config=x86_64 //:all
```

### 运行示例

各模块的示例代码位于 `examples/` 目录下：

```bash
# 查看所有示例
ls examples/

# 运行消息队列示例
bazel run //examples/itc/message_queue:basic_example

# 运行模块加载示例
bazel run //examples/module:loader_example

# 运行并发示例
bazel run //examples/concurrent:mutex_example
bazel run //examples/concurrent:thread_example

# 运行对象示例
bazel run //examples/object:singleton_example
bazel run //examples/object:lifecycled_object_example
```

## 项目结构

```
cytoskeleton-cpp/
├── include/cytoskeleton/          # 头文件目录
│   ├── concurrent/                # 并发模块
│   │   ├── README.md
│   │   ├── concurrent.h           # 统一包含头
│   │   ├── mutex.h
│   │   ├── event.h
│   │   ├── thread.h
│   │   └── thread_pool.h
│   ├── object/                    # 对象模块
│   │   ├── README.md
│   │   ├── object.h               # Object 基类
│   │   ├── lifecycled_object.h    # 生命周期对象
│   │   └── singleton.h            # 单例模板
│   ├── itc/                       # 进程间通信模块
│   │   └── message_queue/         # 消息队列
│   │       ├── message.h
│   │       ├── handler.h
│   │       ├── looper.h
│   │       └── mq.h
│   └── module/                    # 动态模块加载模块 (非 header-only)
│       ├── loader.h               # Loader 类定义
│       ├── stub.h                 # 注册宏定义
│       └── manifest.h             # 数据结构定义
├── src/module/                    # module 模块源码（需要编译）
│   ├── loader.cc                  # Loader 实现
│   └── stub.cc                    # RegisterModule 等函数实现
├── examples/                      # 示例代码
│   ├── concurrent/                # 并发示例（11 个）
│   ├── object/                    # 对象示例（4 个）
│   ├── itc/message_queue/         # 消息队列示例（1 个）
│   └── module/                    # module 模块示例
├── tests/                         # 单元测试
│   ├── concurrent/                # 并发测试（133 个）
│   ├── object/                    # 对象测试（43 个）
│   ├── itc/message_queue/         # 消息队列测试（35 个）
│   └── module/                    # module 模块测试（6 个）
├── documents/                     # 文档
│   └── architecture/
│       └── module_requirements.md # module 模块需求文档
├── MODULE.bazel                   # Bazel 模块定义
├── BUILD.bazel                    # 根构建文件
├── .bazelrc                       # Bazel 构建配置（使用 EROS Forge 统一配置）
└── README.md                      # 本文档
```

**注意**：构建配置（包括交叉编译工具链）由 [EROS Forge](../../forge/README.md) 统一管理。

## 命名空间

所有组件位于统一命名空间下：

```cpp
com::etrita::eros::cytos::concurrent       // 并发模块
com::etrita::eros::cytos::object           // 对象模块
com::etrita::eros::cytos::itc::message_queue  // 消息队列模块
com::etrita::eros::cytos::module           // 动态模块加载模块
```

## 测试

### 运行所有测试

```bash
# 运行所有测试（本地）
bazel test //tests/...

# 运行特定模块测试
bazel test //tests/concurrent:all
bazel test //tests/object:all
bazel test //tests/itc/message_queue:all

# 详细输出
bazel test //tests/... --test_output=all
```

### 测试状态

| 模块 | 测试数 | 状态 |
|------|--------|------|
| concurrent | 133 | ✅ 通过 |
| object | 43 | ✅ 通过 |
| itc/message_queue | 35 | ✅ 通过 |
| module | 6 | ✅ 通过 |

**总计：217 个测试全部通过**

### 远程测试

交叉编译的示例程序可以部署到 ARM64 目标机进行测试。

```bash
# 部署到远程机器（<user>@<target-host> 替换为实际的目标机地址）
scp bazel-bin/examples/concurrent/*_example <user>@<target-host>:~/eros-examples/
scp bazel-bin/examples/module/* <user>@<target-host>:~/eros-libs/

# 远程运行测试
ssh <user>@<target-host> "cd ~/eros-examples && ./mutex_example"
```

**可测试的示例包括**：
- Concurrent: mutex_example, event_example, vector_example, map_example, hash_map_example, queue_example, list_example, stack_example, thread_example, thread_pool_example, tree_example
- ITC: basic_example
- Object: object_basic_example, lifecycled_object_example, auto_start_example, singleton_example
- Module: loader_example

## 文档

### 使用文档

- [Concurrent 模块使用文档](documents/usage/concurrent_usage.md)
- [Object 模块使用文档](documents/usage/object_usage.md)
- [消息队列使用文档](documents/usage/message_queue_usage.md)
- [Module 模块使用文档](documents/usage/module_usage.md)

### 模块文档

- [Concurrent 模块](include/cytoskeleton/concurrent/README.md)
- [Object 模块](include/cytoskeleton/object/README.md)
- [Message Queue 模块](include/cytoskeleton/itc/message_queue/README.md)
- [Module 模块](include/cytoskeleton/module/README.md)

### 架构设计文档

- [架构设计目录](documents/architecture/)
- [Concurrent 模块需求](documents/architecture/concurrent_requirements.md)
- [Object 模块需求](documents/architecture/object_requirements.md)
- [Message Queue 模块需求](documents/architecture/message_queue_requirements.md)
- [Module 模块需求](documents/architecture/module_requirements.md)

## 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

MIT License

## 联系方式

- 项目主页: https://github.com/etrita/cytoskeleton
- 问题反馈: https://github.com/etrita/cytoskeleton/issues
