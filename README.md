# Cytoskeleton C++ 基础库

Cytoskeleton 是一个面向机器人应用与服务开发的 C++20 基础库，提供并发控制、对象生命周期管理、数据结构等核心功能。

## 设计哲学

### 核心原则

1. **安全性 > 易用性 > 性能**
   - 面向机器人应用，可靠性优先
   - 不面向高性能计算场景

2. **Header-Only 设计**
   - 所有实现都在头文件中
   - 便于集成和部署

3. **现代 C++20**
   - 充分利用 `std::jthread`、`std::stop_token`、concepts 等特性
   - 遵循 Google C++ Style Guide

4. **模块化架构**
   - 各模块独立，可单独使用
   - 清晰的依赖关系

## 模块概览

| 模块 | 说明 | 状态 |
|------|------|------|
| [concurrent](include/cytoskeleton/concurrent/README.md) | 并发控制：线程、锁、事件、线程安全容器 | ✅ 已完成 |
| [object](include/cytoskeleton/object/README.md) | 对象基础：生命周期管理、单例模式 | ✅ 已完成 |

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
    ],
)
```

### 示例代码

```cpp
#include "cytoskeleton/object/auto_start_lifecycled_object.h"
#include <iostream>

using namespace com::etrita::eros::cytos::object;

class MyService : public AutoStartLifecycledObject {
 protected:
  void Run(std::stop_token stop_token) override {
    while (!stop_token.stop_requested()) {
      std::cout << "Service running..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
};

int main() {
  auto service = MyService::Create<MyService>();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  service->Stop();
  return 0;
}
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
│   │   ├── thread_pool.h
│   │   └── ...
│   └── object/                    # 对象模块
│       ├── README.md
│       ├── object.h               # Object 基类
│       ├── lifecycled_object.h    # 生命周期对象
│       ├── auto_start_lifecycled_object.h
│       └── singleton.h            # 单例模板
├── examples/                      # 示例代码
│   ├── concurrent/                # 并发模块示例
│   └── object/                    # 对象模块示例
├── tests/                         # 单元测试
│   ├── concurrent/
│   └── object/
├── docs/                          # 文档
│   └── architecture/              # 架构设计文档
├── MODULE.bazel                   # Bazel 模块定义
├── BUILD.bazel                    # 根构建文件
└── README.md                      # 本文档
```

## 命名空间

所有组件位于统一命名空间下：

```cpp
com::etrita::eros::cytos::concurrent  // 并发模块
com::etrita::eros::cytos::object      // 对象模块
```

## 测试

### 运行所有测试

```bash
# 运行所有测试
bazel test //tests/...

# 运行特定模块测试
bazel test //tests/concurrent:all
bazel test //tests/object:all

# 详细输出
bazel test //tests/... --test_output=all
```

### 测试状态

| 模块 | 测试数 | 状态 |
|------|--------|------|
| concurrent | 133 | ✅ 通过 |
| object | 43 | ✅ 通过 |

## 文档

- [Concurrent 模块文档](include/cytoskeleton/concurrent/README.md)
- [Object 模块文档](include/cytoskeleton/object/README.md)
- [架构设计文档](docs/architecture/)

## 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

MIT License

## 联系方式

- 项目主页: https://github.com/etrita/cytoskeleton
- 问题反馈: https://github.com/etrita/cytoskeleton/issues
