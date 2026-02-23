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

#### 编译特定模块
bazel build //include/cytoskeleton/concurrent:all
bazel build //include/cytoskeleton/object:all
bazel build //include/cytoskeleton/itc/message_queue:all
bazel build //include/cytoskeleton/module:all

# 编译示例程序
bazel build //examples/...
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
│   ├── concurrent/
│   ├── object/
│   ├── itc/message_queue/
│   └── module/                    # module 模块示例
├── tests/                         # 单元测试
│   ├── concurrent/
│   ├── object/
│   ├── itc/message_queue/
│   └── module/                    # module 模块测试
├── documents/                     # 文档
│   └── architecture/
│       └── module_requirements.md # module 模块需求文档
├── MODULE.bazel                   # Bazel 模块定义
├── BUILD.bazel                    # 根构建文件
└── README.md                      # 本文档
```

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
# 运行所有测试
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

## 文档

- [Concurrent 模块文档](include/cytoskeleton/concurrent/README.md)
- [Object 模块文档](include/cytoskeleton/object/README.md)
- [消息队列使用文档](documents/architecture/message_queue_usage.md)
- [Module 模块文档](include/cytoskeleton/module/README.md)
- [Module 模块需求文档](documents/architecture/module_requirements.md)
- [架构设计文档](documents/architecture/)

## 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

MIT License

## 联系方式

- 项目主页: https://github.com/etrita/cytoskeleton
- 问题反馈: https://github.com/etrita/cytoskeleton/issues
