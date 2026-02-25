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

### 交叉编译（ARM64）

项目支持 ARM64 交叉编译，使用动态链接方式。对于 glibc 版本不兼容的目标机，可以通过 `LD_LIBRARY_PATH` 指定自定义 glibc 路径。

#### 交叉编译全部目标

```bash
# 交叉编译所有目标（示例、测试、模块库）
bazel build --config=arm64 //:all

# 编译所有示例
bazel build --config=arm64 //examples/...

# 编译所有测试
bazel build --config=arm64 //tests/...

# 编译所有模块库
bazel build --config=arm64 //include/cytoskeleton/module:all
```

#### 安装交叉编译工具链

在 Ubuntu/Debian 系统上安装 ARM64 交叉编译工具链：

```bash
# 安装交叉编译器（要求 GCC 11+ 以支持 C++20）
sudo apt-get update
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 验证安装和版本（需要支持 -std=c++20）
aarch64-linux-gnu-g++ --version
aarch64-linux-gnu-g++ -std=c++20 -dM -E - < /dev/null | grep __cplusplus
```

**版本要求**：
- GCC 11+ （推荐 GCC 13+）
- 必须支持 C++20 标准（`-std=c++20` 或 `-std=gnu++20`）

**验证 C++20 支持**：
```bash
# 检查是否支持 C++20
aarch64-linux-gnu-g++ -std=c++20 -E -xc++ - </dev/null >/dev/null 2>&1 && echo "C++20 supported" || echo "C++20 NOT supported"
```

#### 部署到目标机（使用自定义 glibc）

如果目标机的 glibc 版本较低（如 Ubuntu 18.04 的 glibc 2.27），可以将宿主机的高版本 glibc 和其他依赖的动态库复制到目标机，通过 `LD_LIBRARY_PATH` 指定：

**步骤 1：在宿主机上编译**
```bash
bazel build --config=arm64 //:all
```

**步骤 2：复制系统动态库到目标机**

需要复制以下类型的动态库到目标机：
- **glibc 库**：libc.so.6、libm.so.6、libpthread.so.0、libdl.so.2、librt.so.1、ld-linux-aarch64.so.1
- **编译器运行时库**：libstdc++.so.6、libgcc_s.so.1
- **Bazel 生成的共享库**：编译过程中生成的 `_solib_aarch64` 目录下的库文件
- **其他动态库**：如 `*.so` 文件

```bash
# 在目标机上创建目录（<target-host> 替换为目标机地址，<user> 替换为用户名）
ssh <user>@<target-host> "mkdir -p ~/eros-libs"

# 复制系统 glibc 和编译器库
scp /usr/aarch64-linux-gnu/lib/ld-linux-aarch64.so.1 \
    /usr/aarch64-linux-gnu/lib/libc.so.6 \
    /usr/aarch64-linux-gnu/lib/libm.so.6 \
    /usr/aarch64-linux-gnu/lib/libpthread.so.0 \
    /usr/aarch64-linux-gnu/lib/libdl.so.2 \
    /usr/aarch64-linux-gnu/lib/librt.so.1 \
    /usr/lib/aarch64-linux-gnu/libstdc++.so.6 \
    /usr/lib/aarch64-linux-gnu/libgcc_s.so.1 \
    <user>@<target-host>:~/eros-libs/
```

**步骤 3：复制程序和 Bazel 生成的共享库到目标机**
```bash
# 创建目标目录
ssh <user>@<target-host> "mkdir -p ~/eros-examples"

# 复制示例程序
scp bazel-bin/examples/concurrent/*_example <user>@<target-host>:~/eros-examples/
scp bazel-bin/examples/object/*_example <user>@<target-host>:~/eros-examples/
scp bazel-bin/examples/itc/message_queue/*_example <user>@<target-host>:~/eros-examples/
scp bazel-bin/examples/module/loader_example <user>@<target-host>:~/eros-examples/

# 复制 Bazel 生成的共享库（_solib_aarch64 目录）
scp -r bazel-bin/examples/module/_solib_aarch64 <user>@<target-host>:~/eros-libs/

# 复制其他动态库（如有）
scp bazel-bin/examples/module/*.so <user>@<target-host>:~/eros-libs/
```

**步骤 4：在目标机上运行（使用自定义 glibc）**
```bash
# 设置库路径（包括系统库、Bazel 生成的共享库和其他动态库）
export LD_LIBRARY_PATH="$HOME/eros-libs:$HOME/eros-libs/_solib_aarch64"

# 使用 LD_LIBRARY_PATH 运行程序（通过自定义 glibc 加载器）
$HOME/eros-libs/ld-linux-aarch64.so.1 $HOME/eros-examples/mutex_example
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
├── toolchain/                     # Bazel 交叉编译工具链配置
│   ├── BUILD.bazel                # 工具链定义
│   └── cc_toolchain_config.bzl    # 工具链配置规则
├── examples/                      # 示例代码
│   ├── concurrent/                # 并发示例（11个）
│   ├── object/                    # 对象示例（4个）
│   ├── itc/message_queue/         # 消息队列示例（1个）
│   └── module/                    # module 模块示例
├── tests/                         # 单元测试
│   ├── concurrent/                # 并发测试（133个）
│   ├── object/                    # 对象测试（43个）
│   ├── itc/message_queue/         # 消息队列测试（35个）
│   └── module/                    # module 模块测试（6个）
├── documents/                     # 文档
│   └── architecture/
│       └── module_requirements.md # module 模块需求文档
├── MODULE.bazel                   # Bazel 模块定义
├── BUILD.bazel                    # 根构建文件
├── .bazelrc                       # Bazel 构建配置（含交叉编译配置）
└── README.md                      # 本文档
```

## 交叉编译工具链配置

项目已配置完整的 Bazel 交叉编译工具链，支持：

- **目标平台**：ARM64 (aarch64-linux-gnu)
- **C++标准**：C++20
- **链接方式**：动态链接（支持共享库）
- **GLIBC兼容**：通过 `LD_LIBRARY_PATH` 指定自定义 glibc 路径

### 工具链文件

- `.bazelrc` - Bazel 构建配置文件
- `toolchain/BUILD.bazel` - 工具链定义
- `toolchain/cc_toolchain_config.bzl` - 工具链配置规则

### 使用方法

```bash
# 交叉编译（动态链接）
bazel build --config=arm64 //examples/...

# 本地编译（用于本机测试）
bazel build --config=native //examples/...
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

交叉编译的示例程序可以部署到 ARM64 目标机进行测试。例如部署到 Ubuntu 18.04 ARM64 目标机：

```bash
# 部署到远程机器（<user>@<target-host> 替换为实际的目标机地址）
scp bazel-bin/examples/concurrent/*_example <user>@<target-host>:~/eros-examples/
scp bazel-bin/examples/module/* <user>@<target-host>:~/eros-libs/

# 远程运行测试（使用自定义 glibc）
ssh <user>@<target-host> "cd ~/eros-examples && LD_LIBRARY_PATH=~/eros-libs ~/eros-libs/ld-linux-aarch64.so.1 ./mutex_example"
```

**可测试的示例包括**：
- Concurrent: mutex_example, event_example, vector_example, map_example, hash_map_example, queue_example, list_example, stack_example, thread_example, thread_pool_example, tree_example
- ITC: basic_example
- Object: object_basic_example, lifecycled_object_example, auto_start_example, singleton_example
- Module: loader_example

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
