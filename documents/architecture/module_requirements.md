# Cytoskeleton C++ 基础库 - Module 模块需求文档

## 1. 概述

### 1.1 模块名称
`module` - 动态模块加载与管理模块

### 1.2 命名空间
`com::etrita::eros::cytos::module`

### 1.3 设计目标
- 提供动态库（.so）的运行时加载与管理能力
- 实现基于插件架构的模块注册与发现机制
- 支持模块元数据（名称、版本、类别、属性）的声明与查询
- 提供模块实例化接口，支持按库、按类别、按名称实例化
- **支持 lambda 过滤器，允许用户自定义实例化条件**
- **面向机器人应用与服务开发，支持插件化架构**
- **API 设计原则：安全性 > 易用性 > 性能**

### 1.4 C++ 标准
C++20

### 1.5 命名约定
- 类名：大驼峰命名（PascalCase），如 `Loader`、`ModuleInfo`
- 方法名：大驼峰命名（PascalCase），如 `InstantiateAll`、`LoadManifest`
- 参数名：小驼峰命名（camelCase），如 `library_paths`、`filter`

---

## 2. 核心概念

### 2.1 模块（Module）
模块是动态库中注册的可实例化单元，每个模块包含：
- **名称（name）**：模块的唯一标识符
- **版本（version）**：模块版本号
- **类别（category）**：模块分类，用于按类别查询和实例化
- **属性（properties）**：键值对形式的扩展元数据
- **创建函数（creator）**：用于实例化模块对象的工厂函数，返回 `Object*` 原始指针
- **删除函数（deleter）**：用于销毁模块对象的函数，接收 `Object*` 指针并负责释放

### 2.2 清单（Manifest）
清单是动态库的元数据描述，包含：
- **库文件名（library_filename）**：动态库的路径
- **模块列表（modules）**：库中注册的所有模块信息（`ModuleInfo`）

**序列化说明：**
- 动态库中的清单以 `Manifest` 结构体形式存储（`__manifest__`）
- `GetManifest()` 函数在调用时将 `__manifest__` 序列化为 JSON 字符串并返回
- Loader 解析 JSON 字符串并反序列化为 `Manifest` 结构体

**模块存储说明：**
- `Manifest` 结构体仅存储模块信息（`ModuleInfo`），不包含工厂函数指针
- 动态库内部维护 `__modules__` 向量，存储完整的 `Module` 对象（包含 `creator`）
- `GetModuleCreator(name)` 函数用于从 `__modules__` 中查找指定模块，返回其 `creator` 函数指针
- `creator` 函数指针定义：无参数，返回 `Object*` 原始指针
- `deleter` 函数指针定义：接收 `Object*` 参数，无返回值，负责释放对象

### 2.3 加载器（Loader）
加载器负责：
- 扫描指定目录下的动态库
- 加载动态库并解析其清单
- 管理动态库句柄
- 提供模块实例化接口

**注意：** 本模块默认不支持动态库卸载，动态库加载后将在进程生命周期内保持加载状态。

### 2.4 架构关系
```
┌─────────────────────────────────────────────────────────────┐
│                         Loader                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  manifests_: Map<library_filename, Manifest>          │  │
│  │    - Manifest.modules: Map<name, ModuleInfo>          │  │
│  │  loaded_libs_: Map<library_filename, LoadedLibrary>   │  │
│  │    - LoadedLibrary: {handle, get_module_creator, manifest} │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  InstantiateAll() / Instantiate() → 所有模块实例              │
│  Instantiate(filter) → 符合过滤条件的模块实例                 │
│  Instantiate(lib) → 指定库的模块实例                          │
│  InstantiateByCategory(cat) → 指定类别的模块实例              │
│  InstantiateByName(lib, name) → 指定名称的单模块实例          │
│                                                              │
│  ModuleFilter = std::function<bool(const std::string&,      │
│                                     const ModuleInfo&)>      │
│  - 参数 1: library_filename (动态库路径)                      │
│  - 参数 2: ModuleInfo (模块信息)                              │
│  - 返回：true=实例化，false=跳过                              │
│                                                              │
│  [JSON Parser] ← GetManifest() 返回 JSON 字符串               │
│  GetModuleCreator(name) ← 返回 creator 函数指针 (Object* (*)()) │
│    - 参数：const char* module_name                               │
└─────────────────────────────────────────────────────────────┘
                                │
                                │ 加载/管理
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                      Dynamic Library (.so)                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  __manifest__: Manifest                               │  │
│  │    - modules: Map<name, ModuleInfo>                   │  │
│  │                                                       │  │
│  │  __modules__: std::vector<Module>                      │  │
│  │    - Module: {ModuleInfo, creator, deleter}            │  │
│  │    - GetModuleCreator(name) 从此向量查找并返回 creator 和 deleter │  │
│  │                                                       │  │
│  │  GetManifest(): 调用时实时序列化 __manifest__ 为 JSON 字符串  │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  ModuleA (类，继承 Object)                             │  │
│  │    - DEFINE_MODULE_FACTORY(ModuleA)                   │  │
│  │    - REGISTER_MODULE(...)                             │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  ModuleB (类，继承 Object)                             │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. 数据结构

### 3.1 ModuleInfo（模块信息）

```cpp
struct ModuleInfo {
  std::string name;
  std::string version;
  std::string category;
  std::string description;
  std::string author;
  std::vector<std::string> dependencies;
  std::unordered_map<std::string, std::string> properties;
};
```

**字段说明：**
- `name`：模块名称，作为模块的唯一标识
- `version`：模块版本号，格式建议为语义化版本（如 "1.0.0"）
- `category`：模块类别，用于分类查询（如 "service/data_service"）
- `description`：模块描述，用于人机阅读的简短说明
- `author`：作者信息，记录模块开发者或团队名称
- `dependencies`：依赖列表，记录该模块依赖的其他模块名称
- `properties`：扩展属性，用于存储自定义元数据

### 3.2 Module（模块）

```cpp
struct Module {
  ModuleInfo info;
  Object* (*creator)();
  void (*deleter)(Object*);
};
```

**字段说明：**
- `info`：模块信息
- `creator`：工厂函数指针，无参数，返回 `Object*` 原始指针
- `deleter`：删除函数指针，接收 `Object*` 参数，负责释放对象（调用 delete 或自定义释放逻辑）

### 3.3 Manifest（清单）

```cpp
struct Manifest {
  std::string library_filename;
  std::unordered_map<std::string, ModuleInfo> modules;
};
```

**字段说明：**
- `library_filename`：动态库的完整路径
- `modules`：库中注册的所有模块信息，键为模块名称（仅包含 `ModuleInfo`，不包含工厂函数指针）

**动态库内部存储：**
动态库内部额外维护一个 `__modules__` 向量，用于存储完整的 `Module` 对象（包含 `creator`）：

```cpp
static std::vector<Module> __modules__;
```

**说明：**
- `Manifest` 用于序列化和跨库传递（仅包含元数据）
- `__modules__` 用于在动态库内部存储工厂函数指针，通过 `GetModuleCreator()` 访问
- `GetManifest()` 在调用时将 `__manifest__` 实时序列化为 JSON 字符串

---

## 4. Loader 类

### 4.1 设计说明

`Loader` 类是模块系统的核心，负责动态库的加载、管理和模块实例化。

**核心功能：**
- 扫描指定目录下的所有 .so 文件
- 使用 `dlopen` 加载动态库
- 获取动态库的 `GetManifest` 函数返回的 JSON 字符串
- 解析 JSON 字符串，反序列化为 `Manifest` 结构体
- 管理动态库句柄
- 提供多种模块实例化方式

**注意：** 默认不支持动态库卸载，加载的动态库将在进程生命周期内保持加载状态。

**线程安全：**
- 使用并发安全的 `Map` 存储清单和句柄
- 支持多线程环境下的模块查询和实例化

### 4.2 API

**构造函数：**
```cpp
explicit Loader(const std::vector<std::string>& library_paths,
                const std::string& filter = {});
```
- `library_paths`：要扫描的目录列表
- `filter`：可选的过滤器，只加载文件名包含过滤器的库

**析构函数：**
```cpp
virtual ~Loader() = default;
```
- 默认析构函数，不执行卸载操作（动态库默认不卸载）

**类型定义：**

```cpp
// 模块过滤器：接收库文件名和模块信息，返回是否匹配
// 如果过滤器返回 true，则实例化该模块；返回 false 则跳过
// 空过滤器（nullptr 或空 std::function）表示匹配所有模块
using ModuleFilter = std::function<bool(const std::string&, const ModuleInfo&)>;
```

**公共方法：**

```cpp
// 实例化所有模块（等价于 Instantiate() 无过滤器）
virtual std::vector<Object::Ptr> InstantiateAll();

// 使用过滤器实例化模块
// filter: 可选的过滤器，空过滤器表示实例化所有模块
virtual std::vector<Object::Ptr> Instantiate(
    const ModuleFilter& filter = {});

// 实例化指定库的所有模块
// library_filename: 动态库的完整路径
virtual std::vector<Object::Ptr> Instantiate(
    const std::string& library_filename);

// 实例化指定类别的所有模块
// category: 模块类别，如 "service/data_service"
virtual std::vector<Object::Ptr> InstantiateByCategory(
    const std::string& category);

// 实例化指定库中的指定模块
// library_filename: 动态库的完整路径
// module_name: 模块名称
virtual Object::Ptr InstantiateByName(
    const std::string& library_filename,
    const std::string& module_name);

// 打印已加载模块的详细信息
virtual void Dump();
```

### 4.3 使用示例

```cpp
#include "cytoskeleton/module/loader.h"

using namespace com::etrita::eros::cytos::module;

// 创建加载器，扫描当前目录
auto path = std::filesystem::current_path();
Loader loader({path.string()});

// 打印已加载模块信息
loader.Dump();

// 实例化所有模块（无过滤器）
auto all_objects = loader.InstantiateAll();
// 或者：auto all_objects = loader.Instantiate();
for (const auto& object : all_objects) {
  std::cout << "object id: " << object->Id() << std::endl;
}

// 使用 lambda 过滤器实例化：按类别过滤
auto service_objects = loader.Instantiate(
    [](const std::string& lib, const ModuleInfo& info) {
      return info.category == "service/data_service";
    });

// 使用 lambda 过滤器实例化：按库名和版本过滤
auto filtered_objects = loader.Instantiate(
    [](const std::string& lib, const ModuleInfo& info) {
      return lib.find("service") != std::string::npos &&
             info.version >= "1.0.0";
    });

// 使用 lambda 过滤器实例化：按属性过滤
auto enabled_objects = loader.Instantiate(
    [](const std::string& lib, const ModuleInfo& info) {
      return info.properties.count("enabled") &&
             info.properties.at("enabled") == "true";
    });

// 实例化指定库的所有模块
auto lib_objects = loader.Instantiate("/path/to/libmy_module.so");

// 实例化指定类别的模块（便捷方法）
auto by_category = loader.InstantiateByCategory("service/data_service");

// 实例化指定模块
auto single_object = loader.InstantiateByName(
    "/path/to/libmy_module.so", "MyModule");
```

---

## 5. 动态库开发指南

### 5.1 宏定义

**REGISTER_MODULE**
用于注册模块到清单。由于 `DEFINE_MODULE_FACTORY` 已经定义了 `Create` 和 `Deleter` 方法，此宏会自动推导并使用它们：

```cpp
#define REGISTER_MODULE(name, version, category, description, author, dependencies) \
  static void REGISTER_MODULE_##name(void) __attribute__((         \
      constructor(INITIALIZE_REGISTER_MODULE_PRIORITY)));          \
  static void REGISTER_MODULE_##name(void) {                       \
    RegisterModule(#name, version, category, description, author, dependencies,    \
                   name::Create, name::Deleter);                   \
  }
```

**参数说明：**
- `name`：模块名称（类名）
- `version`：版本号
- `category`：类别
- `description`：描述
- `author`：作者
- `dependencies`：依赖列表

**自动推导说明：**
- `create_func` 自动推导为 `name::Create`
- `delete_func` 自动推导为 `name::Deleter`
- 要求模块类必须使用 `DEFINE_MODULE_FACTORY` 宏定义 `Create` 和 `Deleter` 方法

### 5.1.1 跨编译器静态初始化

为实现"定义即注册"，需要在动态库加载时自动执行注册函数。不同编译器提供不同的静态初始化机制：

**GCC/Clang (Linux, macOS)**
```cpp
// 使用 __attribute__((constructor))
#define REGISTER_MODULE(name, version, category, description, author, dependencies) \
  static void REGISTER_MODULE_##name(void) __attribute__((         \
      constructor(INITIALIZE_REGISTER_MODULE_PRIORITY)));          \
  static void REGISTER_MODULE_##name(void) {                       \
    RegisterModule(#name, version, category, description, author, dependencies,    \
                   name::Create, name::Deleter);                   \
  }

// 优先级范围：101-65535，数值越小优先级越高
// 默认优先级：101（最高优先级，确保在其他全局对象之前初始化）
#ifndef INITIALIZE_REGISTER_MODULE_PRIORITY
#define INITIALIZE_REGISTER_MODULE_PRIORITY 101
#endif
```

**MSVC (Windows)**
```cpp
// 使用 #pragma section 和 .CRT$XCU 段
#define REGISTER_MODULE(name, version, category, description, author, dependencies) \
  static void REGISTER_MODULE_##name(void);                                        \
  namespace {                                                                      \
    struct REGISTER_MODULE_##name##_Struct {                                       \
      REGISTER_MODULE_##name##_Struct() { REGISTER_MODULE_##name(); }              \
    };                                                                             \
    static REGISTER_MODULE_##name##_Struct REGISTER_MODULE_##name##_Instance;      \
  }                                                                                \
  static void REGISTER_MODULE_##name(void) {                                       \
    RegisterModule(#name, version, category, description, author, dependencies,    \
                   name::Create, name::Deleter);                                   \
  }
```

**跨平台统一宏定义**
```cpp
#ifdef _MSC_VER
  // MSVC 编译器
  #define REGISTER_MODULE(name, version, category, description, author, dependencies) \
    static void REGISTER_MODULE_##name(void);                                        \
    namespace {                                                                      \
      struct REGISTER_MODULE_##name##_Struct {                                       \
        REGISTER_MODULE_##name##_Struct() { REGISTER_MODULE_##name(); }              \
      };                                                                             \
      static REGISTER_MODULE_##name##_Struct REGISTER_MODULE_##name##_Instance;      \
    }                                                                                \
    static void REGISTER_MODULE_##name(void) {                                       \
      RegisterModule(#name, version, category, description, author, dependencies,    \
                     name::Create, name::Deleter);                                   \
    }
#else
  // GCC/Clang 及其他支持 __attribute__((constructor)) 的编译器
  #ifndef INITIALIZE_REGISTER_MODULE_PRIORITY
  #define INITIALIZE_REGISTER_MODULE_PRIORITY 101
  #endif
  #define REGISTER_MODULE(name, version, category, description, author, dependencies) \
    static void REGISTER_MODULE_##name(void) __attribute__((         \
        constructor(INITIALIZE_REGISTER_MODULE_PRIORITY)));          \
    static void REGISTER_MODULE_##name(void) {                       \
      RegisterModule(#name, version, category, description, author, dependencies,    \
                     name::Create, name::Deleter);                   \
    }
#endif
```

**编译器支持说明：**

| 编译器 | 机制 | 支持优先级 | 备注 |
|--------|------|------------|------|
| GCC | `__attribute__((constructor(priority)))` | 是 | 优先级范围：101-65535 |
| Clang | `__attribute__((constructor(priority)))` | 是 | 兼容 GCC 语法 |
| MSVC | `#pragma section` / 静态对象构造函数 | 否 | 通过 C++ 静态对象构造函数实现 |
| ICC (Intel) | `__attribute__((constructor))` | 是 | 兼容 GCC 语法 |
| ARM Compiler | `__attribute__((constructor))` | 是 | 用于嵌入式 ARM 平台 |
| IAR | `#pragma section` | 否 | 需要特殊处理 |

**注意：**
- 本设计主要面向 GCC/Clang 编译器（Linux 平台机器人应用）
- 如需支持 MSVC（Windows 平台），可使用上述跨平台宏定义
- 优先级 101 确保注册函数在其他全局对象之前执行，避免依赖问题

### 5.1.2 跨编译器实现原理

**问题：为什么需要跨编译器支持？**

"定义即注册"机制依赖于动态库加载时自动执行注册函数。不同编译器提供不同的静态初始化机制：

1. **GCC/Clang**: 使用 `__attribute__((constructor))`
2. **MSVC**: 使用静态对象构造函数（`.CRT$XCU` 段）
3. **ICC (Intel)**: 兼容 GCC 语法
4. **其他编译器**: ARM Compiler、IAR 等也有类似机制

**实现方式对比：**

| 机制 | GCC/Clang | MSVC | ICC | ARM Compiler |
|------|-----------|------|-----|--------------|
| `__attribute__((constructor))` | ✓ | ✗ | ✓ | ✓ |
| 静态对象构造函数 | ✓ | ✓ | ✓ | ✓ |
| 优先级控制 | ✓ (101-65535) | ✗ | ✓ | ✓ |

**结论：**
- 所有主流编译器都支持"定义即注册"机制
- 实现方式可能不同（`__attribute__` 或静态对象构造函数）
- 通过条件编译（`#ifdef _MSC_VER`）可以统一 API
- 优先级控制是可选的，不影响基本功能

**推荐做法：**
```cpp
// 在 stub.h 中定义跨平台宏
#ifdef _MSC_VER
  // MSVC: 使用静态对象构造函数
  #define REGISTER_MODULE(name, ...) /* MSVC 实现 */
#else
  // GCC/Clang/ICC: 使用 __attribute__((constructor))
  #define REGISTER_MODULE(name, ...) /* GCC 实现 */
#endif
```

**动态库加载时的执行顺序：**
```
1. dlopen() 加载动态库
   │
   ├─► 加载 .data 段（全局变量）
   ├─► 加载 .bss 段（未初始化全局变量）
   ├─► 执行构造函数段
   │   ├─► GCC/Clang: .init_array (包含 __attribute__((constructor)) 函数)
   │   └─► MSVC: .CRT$XCU (静态对象构造函数)
   │       │
   │       └─► REGISTER_MODULE 宏生成的注册函数被调用
   │           └─► RegisterModule() 被调用
   │               └─► __manifest__ 和 __modules__ 被填充
   │
   └─► dlopen() 返回，动态库初始化完成
```

**DEFINE_MODULE_FACTORY**
用于在类中定义静态工厂方法和删除器：

```cpp
#define DEFINE_MODULE_FACTORY(classname)       \
  static classname* Create() {                 \
    return new classname();                    \
  }                                            \
  static void Deleter(Object* obj) {           \
    delete static_cast<classname*>(obj);       \
  }
```

**说明：**
- `Create()`：工厂方法，返回 `Object*` 原始指针
- `Deleter(Object*)`：删除器，负责释放对象（使用 `delete`）

**REGISTER_MODULE 简化宏（可选）**
为方便使用，可提供不带可选参数的简化宏：

```cpp
#define REGISTER_MODULE_SIMPLE(name, version, category, description) \
  REGISTER_MODULE(name, version, category, description, "", {})
```

### 5.2 动态库开发步骤

**步骤 1：定义模块类**

模块类必须继承自 `Object` 基类：

```cpp
#include "cytoskeleton/object/object.h"
#include "cytoskeleton/module/stub.h"

using namespace com::etrita::eros::cytos::object;

class MyModule : public Object {
 public:
  DEFINE_MODULE_FACTORY(MyModule);
  
  MyModule() {
    SetId("MyModule");
    // 初始化逻辑
  }
  
  // 模块业务逻辑
  void DoWork() {
    // ...
  }
};
```

**步骤 2：注册模块**

使用 `REGISTER_MODULE` 宏注册模块：

```cpp
REGISTER_MODULE(MyModule, "1.0.0", "service/my_service",
                "我的模块描述", "作者名", {});

// 或使用简化宏
REGISTER_MODULE_SIMPLE(MyModule, "1.0.0", "service/my_service",
                       "我的模块描述");
```

**说明：**
- `REGISTER_MODULE` 宏会自动推导 `MyModule::Create` 和 `MyModule::Deleter`
- 要求模块类必须使用 `DEFINE_MODULE_FACTORY` 宏定义这两个方法

**步骤 3：编译为动态库**

```bash
g++ -shared -fPIC -o libmy_module.so my_module.cc \
    -lcytoskeleton_object -lcytoskeleton_module
```

### 5.3 完整示例

**dynamic_library1.cc**
```cpp
#include <iostream>
#include "cytoskeleton/module/stub.h"
#include "cytoskeleton/object/object.h"

using namespace com::etrita::eros::cytos::object;

// 定义模块 A
class ModuleA : public Object {
 public:
  DEFINE_MODULE_FACTORY(ModuleA);
  ModuleA() {
    SetId("ModuleA");
    std::cout << "ModuleA constructor" << std::endl;
  }
};

REGISTER_MODULE_SIMPLE(ModuleA, "1.0.0", "service/data_service",
                       "数据服务模块 A");

// 定义模块 B
class ModuleB : public Object {
 public:
  DEFINE_MODULE_FACTORY(ModuleB);
  ModuleB() {
    SetId("ModuleB");
    std::cout << "ModuleB constructor" << std::endl;
  }
};

REGISTER_MODULE_SIMPLE(ModuleB, "1.0.0", "service/data_service",
                       "数据服务模块 B");
```

**loader.cc（主程序）**
```cpp
#include "cytoskeleton/module/loader.h"
#include <filesystem>
#include <iostream>

using namespace com::etrita::eros::cytos::module;

int main(int argc, char** argv) {
  auto path = std::filesystem::current_path();
  std::cout << "Current path: " << path.string() << std::endl;

  // 创建加载器，扫描当前目录
  Loader loader({path.string()});
  
  // 打印模块信息
  loader.Dump();
  
  // 实例化所有模块
  auto objects = loader.InstantiateAll();
  for (const auto& object : objects) {
    std::cout << "object id: " << object->Id() << std::endl;
  }

  return 0;
}
```

---

## 6. 实现细节

### 6.1 动态库加载流程

```
1. Loader 构造函数
   │
   ▼
2. 遍历 library_paths
   │
   ▼
3. 扫描目录下的 .so 文件
   │
   ▼
4. 应用 filter 过滤（如果提供）
   │
   ▼
5. 调用 LoadManifest()
   │
   ├─► dlopen() 加载动态库
   ├─► dlsym() 获取 GetManifest 符号
   ├─► 调用 GetManifest() 获取 JSON 字符串
   ├─► 解析 JSON 字符串，反序列化为 Manifest 结构体
   ├─► dlsym() 获取 GetModuleCreator 符号
   └─► 缓存到 loaded_libs_：{handle, get_module_creator, manifest}
   │
   ▼
6. 继续处理下一个库
   │
   ▼
7. 继续处理下一个库
```

### 6.5 LoadedLibrary 结构体（优化）

为避免每次实例化时都调用 `dlsym()` 查找函数，Loader 在加载阶段缓存函数指针：

```cpp
// GetModuleCreator 函数指针类型
using GetModuleCreatorFunc = void(*)(const char*, Object*(**)(), void(**)(Object*));

// GetManifest 函数指针类型
using GetManifestFunc = const char*(*)();

// 缓存已加载的动态库信息
struct LoadedLibrary {
  void* handle;                    // dlopen() 返回的句柄
  GetManifestFunc get_manifest;    // GetManifest 函数指针
  GetModuleCreatorFunc get_module_creator;  // GetModuleCreator 函数指针
  Manifest manifest;               // 反序列化后的 Manifest
};
```

**说明：**
- `handle`：动态库句柄，用于后续可能的卸载操作
- `get_manifest`：缓存 `GetManifest` 函数指针，避免重复 `dlsym()`
- `get_module_creator`：缓存 `GetModuleCreator` 函数指针，避免重复 `dlsym()`
- `manifest`：已解析的 `Manifest` 结构体，避免重复解析 JSON

### 6.6 模块实例化流程（优化后）

```
1. Loader.InstantiateAll() / Instantiate(lib) / InstantiateByCategory(cat)
   │
   ▼
2. 遍历 loaded_libs_ 中的 LoadedLibrary（已缓存函数指针）
   │
   ▼
3. 对于每个 LoadedLibrary.manifest.modules 中的模块
   │
   ├─► 直接使用缓存的 get_module_creator(module_name, &creator, &deleter)
   ├─► 如果 creator != nullptr 且 deleter != nullptr：
   │   ├─► 调用 creator() 创建模块实例（Object* 原始指针）
   │   └─► 使用 deleter 创建 std::shared_ptr<Object>(raw_ptr, deleter)
   ├─► 将 std::shared_ptr<Object> 添加到返回向量
   └─► 继续处理下一个模块
   │
   ▼
4. 返回所有模块实例
```

**优化对比：**

| 操作 | 优化前 | 优化后 |
|------|--------|--------|
| 加载阶段 | `dlsym(GetManifest)` + `dlsym(GetModuleCreator)` | `dlsym(GetManifest)` + `dlsym(GetModuleCreator)` + 缓存 |
| 实例化阶段 | 每次遍历都 `dlsym(GetModuleCreator)` | 直接使用缓存的函数指针 |
| 性能 | O(n) 次 `dlsym()` 调用 | O(1) 次 `dlsym()` 调用 |

**实例化示例代码（优化后）：**
```cpp
// 加载阶段（在 LoadManifest() 中）
using GetManifestFunc = const char*(*)();
using GetModuleCreatorFunc = void(*)(const char*, Object*(**)(), void(**)(Object*));

auto* get_manifest = reinterpret_cast<GetManifestFunc>(
    dlsym(handle, "GetManifest"));
auto* get_module_creator = reinterpret_cast<GetModuleCreatorFunc>(
    dlsym(handle, "GetModuleCreator"));

// 解析 JSON 获取 Manifest
std::string manifest_json = get_manifest();
Manifest manifest = ParseFromJson(manifest_json);

// 缓存到 loaded_libs_
loaded_libs_[library_path] = LoadedLibrary{
    handle, get_manifest, get_module_creator, manifest};

// 实例化阶段（直接使用缓存的函数指针）
std::vector<std::shared_ptr<Object>> instances;
for (const auto& [name, info] : manifest.modules) {
  Object* (*creator)() = nullptr;
  void (*deleter)(Object*) = nullptr;
  
  // 直接使用缓存的 get_module_creator，无需 dlsym()
  get_module_creator(name.c_str(), &creator, &deleter);
  
  if (creator != nullptr && deleter != nullptr) {
    Object* raw_ptr = creator();
    // 使用自定义删除器创建 std::shared_ptr
    instances.push_back(std::shared_ptr<Object>(raw_ptr, deleter));
  }
}
```

### 6.2 GetManifest 函数

动态库必须导出 `GetManifest` 函数，返回清单的 JSON 字符串表示：

```cpp
extern "C" const char* GetManifest();
```

**返回说明：**
- 返回一个 C 风格字符串（`const char*`），内容为 Manifest 的 JSON 序列化表示
- JSON 字符串包含 `library_filename` 和 `modules` 字段
- 返回的字符串必须是静态存储期或由 `strdup` 分配，确保在动态库卸载前有效

**JSON 格式示例：**
```json
{
  "library_filename": "/path/to/libmy_module.so",
  "modules": {
    "ModuleA": {
      "name": "ModuleA",
      "version": "1.0.0",
      "category": "service/data_service",
      "description": "数据服务模块 A",
      "author": "开发团队 A",
      "dependencies": [],
      "properties": {}
    },
    "ModuleB": {
      "name": "ModuleB",
      "version": "1.0.0",
      "category": "service/data_service",
      "description": "数据服务模块 B",
      "author": "开发团队 B",
      "dependencies": ["ModuleA"],
      "properties": {"key1": "value1"}
    }
  }
}
```

**实现说明：**
- 使用 `__attribute__((init_priority))` 确保 `__manifest__` 在注册函数之前构造
- `RegisterModule` 每次注册模块时，需要同步更新 `__manifest__` 结构体
- `GetManifest()` 在调用时实时序列化 `__manifest__` 为 JSON 字符串并返回
- 使用静态 `std::string` 存储 JSON 字符串，确保返回的指针在函数调用期间有效

### 6.3 GetModuleCreator 函数

动态库必须导出 `GetModuleCreator` 函数，用于获取指定模块的工厂函数指针和删除器：

```cpp
extern "C" void GetModuleCreator(
    const char* module_name,
    Object* (**creator)(),
    void (**deleter)(Object*));
```

**参数说明：**
- `module_name`：模块名称，使用 `const char*` 避免跨动态库边界的 STL 兼容性问题
- `creator`：输出参数，用于返回创建器函数指针
- `deleter`：输出参数，用于返回删除器函数指针

**返回说明：**
- 无返回值
- 如果找不到指定名称的模块，`*creator` 和 `*deleter` 均设置为 `nullptr`

**实现说明：**
- 动态库内部维护 `__modules__` 向量，存储所有注册的 `Module` 对象
- `GetModuleCreator(name)` 遍历 `__modules__`，查找 `info.name == module_name` 的模块
- 找到后设置 `*creator = module.creator` 和 `*deleter = module.deleter`

### 6.4 RegisterModule 函数

由 stub 模块提供，用于注册模块：

```cpp
extern "C" void RegisterModule(
    const std::string& name, const std::string& version,
    const std::string& category, const std::string& description,
    const std::string& author, const std::vector<std::string>& dependencies,
    Object* (*creator)(),
    void (*deleter)(Object*));
```

**功能：**
- 将模块信息插入到 `__manifest__.modules` 中（仅 `ModuleInfo`）
- 将完整的 `Module` 对象（包含 `creator` 和 `deleter`）插入到 `__modules__` 向量中
- **不**在注册时序列化，`GetManifest()` 在调用时实时序列化

**实现示例：**
```cpp
// 全局静态变量
static Manifest __manifest__;
static std::vector<Module> __modules__;

extern "C" void RegisterModule(
    const std::string& name, const std::string& version,
    const std::string& category, const std::string& description,
    const std::string& author, const std::vector<std::string>& dependencies,
    Object* (*creator)(),
    void (*deleter)(Object*)) {
  
  // 1. 构建 ModuleInfo
  ModuleInfo info{name, version, category, description, author, dependencies, {}};
  
  // 2. 插入到 __manifest__.modules（仅 ModuleInfo）
  __manifest__.modules[name] = info;
  
  // 3. 插入到 __modules__（完整 Module 对象，包含 creator 和 deleter）
  __modules__.push_back(Module{info, creator, deleter});
  
  // 注意：不在这里序列化，GetManifest() 在调用时实时序列化
}

extern "C" const char* GetManifest() {
  // 实时序列化
  static thread_local std::string json_str;
  json_str = SerializeToJson(__manifest__);
  return json_str.c_str();
}

extern "C" void GetModuleCreator(
    const char* module_name,
    Object* (**creator)(),
    void (**deleter)(Object*)) {
  for (const auto& module : __modules__) {
    if (module.info.name == module_name) {
      *creator = module.creator;
      *deleter = module.deleter;
      return;
    }
  }
  // 未找到模块，返回 nullptr
  *creator = nullptr;
  *deleter = nullptr;
}
```

**注意事项：**
- `__manifest__` 必须是 `static` 变量，确保在动态库生命周期内有效
- `__modules__` 必须是 `static` 向量，确保在动态库生命周期内有效
- `GetManifest()` 使用 `thread_local std::string` 存储序列化结果，确保返回的指针在函数调用期间有效
- 每次调用 `GetManifest()` 都会重新序列化，但避免了在注册时维护额外的字符串状态

---

## 6.7 实现限制：非 Header-only 库

由于 `module` 模块需要在动态库中维护全局状态（`__modules__` 和 `__manifest__`），该模块**不能**实现为纯 header-only 库。

**原因：**
1. `__modules__` 和 `__manifest__` 需要在动态库中维护单一实例
2. 如果使用 `static` 变量在头文件中定义，每个编译单元会生成独立副本
3. 如果使用 `inline` 变量（C++17），虽然可以共享实例，但动态库加载时的符号解析可能不可靠

**实现方式：**
- `RegisterModule`、`GetManifest`、`GetModuleCreator` 函数必须在源文件（.cc）中实现
- 宏定义（`REGISTER_MODULE`、`DEFINE_MODULE_FACTORY`）可以在头文件中
- `Manifest`、`Module`、`ModuleInfo` 结构体可以在头文件中

**文件结构：**
```
include/cytoskeleton/module/
├── loader.h              # Loader 类定义（header-only）
├── stub.h                # 注册宏定义（header-only）
└── manifest.h            # 数据结构定义（header-only）

src/module/
├── loader.cc             # Loader 实现（需要编译）
└── stub.cc               # RegisterModule 等函数实现（需要编译）
```

**编译要求：**
- `cytoskeleton_module` 需要编译为静态库或动态库
- 动态库开发者需要链接 `cytoskeleton_module` 库

**README 说明更新：**
在 `cytoskeleton-cpp` 的 README 中，需要说明：
- `object`、`concurrent` 等模块是 header-only
- `module` 模块**不是** header-only，需要编译链接

## 7. 依赖

- C++20 标准库（包括 `<filesystem>` 用于目录扫描）
- `object` 模块（Object 基类）
- `concurrent` 模块（并发安全的 Map）
- `logger` 模块（日志记录，可选）
- dlfcn.h（动态库加载）
- **JSON 库（如 nlohmann/json 或 RapidJSON，用于清单序列化/反序列化）**

---

## 8. 测试要求

### 8.1 Loader 测试
- 测试扫描目录并加载 .so 文件
- 测试 filter 过滤功能
- 测试加载失败处理（无效库、缺少符号等）

### 8.2 实例化测试
- 测试 `InstantiateAll()` 实例化所有模块
- 测试 `Instantiate()`（无过滤器）实例化所有模块
- 测试 `Instantiate(filter)` 使用过滤器实例化
- 测试 `Instantiate(lib)` 实例化指定库的模块
- 测试 `InstantiateByCategory()` 按类别实例化
- 测试 `InstantiateByName()` 按名称实例化单个模块

### 8.6 过滤器测试
- 测试空过滤器（默认参数）匹配所有模块
- 测试按类别过滤
- 测试按版本过滤
- 测试按属性键值对过滤
- 测试按库文件名过滤
- 测试组合条件过滤

### 8.3 模块注册测试
- 测试单个模块注册
- 测试多个模块注册
- 测试模块元数据正确性
- 测试工厂函数正确创建实例

### 8.4 动态库测试
- 测试 GetManifest 返回正确的 JSON 字符串
- 测试 JSON 字符串格式正确性（可被标准 JSON 库解析）
- 测试 JSON 反序列化后 Manifest 内容正确性
- 测试多个动态库同时加载

### 8.5 GetModuleCreator 测试
- 测试 GetModuleCreator 返回正确的 creator 函数指针
- 测试通过 creator 函数指针创建的模块实例类型正确性
- 测试查找不存在的模块时返回 nullptr

---

## 9. 文件结构

```
include/cytoskeleton/module/
├── loader.h              # Loader 类定义
├── stub.h                # 注册宏和外部接口
└── manifest.h            # Manifest、Module、ModuleInfo 定义（可选）

src/module/
├── loader.cc             # Loader 实现
└── stub.cc               # RegisterModule 实现

tests/module/
├── loader_test.cpp       # Loader 测试
├── module_registration_test.cpp  # 模块注册测试
└── test_modules/         # 测试用动态库
    ├── test_module1.cpp
    └── test_module2.cpp
```

---

## 10. 注意事项

### 10.1 动态库依赖
- 动态库应避免依赖第三方库，以减少兼容性问题
- 如需使用第三方库，应确保版本兼容

### 10.2 符号可见性
- 使用 `extern "C"` 导出 `GetManifest` 和 `GetModuleCreator` 函数
- 避免 C++ 名称修饰导致符号查找失败

### 10.3 资源管理
- 默认不支持动态库卸载，加载的动态库将在进程生命周期内保持加载状态
- 由于清单是从 JSON 字符串反序列化而来，不持有动态库中的资源引用
- `GetManifest()` 使用 `thread_local std::string` 存储序列化结果，确保返回的指针在函数调用期间有效

### 10.4 线程安全
- 使用并发安全的 `Map` 存储清单和句柄
- 模块实例化操作是线程安全的

### 10.5 错误处理
- 动态库加载失败时记录日志并跳过
- 缺少必需符号（GetManifest、GetModuleCreator）时加载失败

### 10.6 设计说明
- `Manifest` 仅包含模块元数据（`ModuleInfo`），用于序列化和跨库传递
- `__modules__` 向量存储完整的 `Module` 对象（包含 `creator` 和 `deleter`），仅在动态库内部使用
- 这种设计分离了元数据和工厂函数/删除器，使得清单可以安全地序列化为 JSON 并传递给 Loader
- Loader 通过 `GetModuleCreator()` 获取工厂函数指针和删除器函数指针

### 10.7 GetModuleCreator 设计
- `GetModuleCreator` 使用输出参数同时返回 `creator` 和 `deleter` 函数指针
- 不使用任何 STL 类型（如 `std::shared_ptr` 或 `std::function`）作为参数或返回类型
- 这避免了跨动态库边界的 STL 兼容性问题

### 10.8 跨动态库边界的对象生命周期管理
- `creator` 在动态库内部创建对象，返回 `Object*` 原始指针
- `deleter` 在动态库内部销毁对象，确保 `delete` 操作在正确的堆上执行
- Loader 使用 `std::shared_ptr<Object>(raw_ptr, deleter)` 包装原始指针
- 当 `std::shared_ptr` 引用计数归零时，会自动调用 `deleter` 释放对象
- 这种设计确保了跨动态库边界的对象能够正确销毁，避免堆不一致问题
