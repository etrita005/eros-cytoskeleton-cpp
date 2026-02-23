# Module 模块使用文档

## 概述

Module 模块提供了动态库（.so）的运行时加载与管理能力，支持基于插件架构的模块注册与发现机制。

## 核心概念

### 模块（Module）
模块是动态库中注册的可实例化单元，包含：
- **名称（name）**：模块的唯一标识符
- **版本（version）**：模块版本号
- **类别（category）**：模块分类
- **属性（properties）**：键值对形式的扩展元数据
- **创建函数（creator）**：用于实例化模块对象的工厂函数
- **删除函数（deleter）**：用于销毁模块对象的函数

### 清单（Manifest）
清单是动态库的元数据描述，包含库文件名和模块列表。

### 加载器（Loader）
加载器负责扫描目录、加载动态库、管理句柄和提供模块实例化接口。

## API 参考

### 数据结构

#### ModuleInfo
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

#### Manifest
```cpp
struct Manifest {
  std::string library_filename;
  std::unordered_map<std::string, ModuleInfo> modules;
};
```

### Loader 类

#### 构造函数
```cpp
explicit Loader(const std::vector<std::string>& library_paths,
                const std::string& filter = {});
```

#### 实例化方法
```cpp
// 实例化所有模块
virtual std::vector<Object::Ptr> InstantiateAll();

// 使用过滤器实例化模块
virtual std::vector<Object::Ptr> Instantiate(
    const ModuleFilter& filter = {});

// 实例化指定库的所有模块
virtual std::vector<Object::Ptr> Instantiate(
    const std::string& library_filename);

// 实例化指定类别的所有模块
virtual std::vector<Object::Ptr> InstantiateByCategory(
    const std::string& category);

// 实例化指定模块
virtual Object::Ptr InstantiateByName(
    const std::string& library_filename,
    const std::string& module_name);
```

#### 调试方法
```cpp
virtual void Dump();
```

## 使用示例

### 1. 创建动态库模块

```cpp
#include "cytoskeleton/module/stub.h"
#include "cytoskeleton/object/object.h"

using namespace com::etrita::eros::cytos::object;
using namespace com::etrita::eros::cytos::module;

class MyService : public Object {
 public:
  DEFINE_MODULE_FACTORY(MyService);

  MyService() {
    SetId("MyService");
  }

  void DoWork() {
    // 业务逻辑
  }
};

REGISTER_MODULE_SIMPLE(MyService, "1.0.0", "service/data_service",
                       "My data service module");
```

### 2. 加载和使用模块

```cpp
#include "cytoskeleton/module/loader.h"
#include <filesystem>
#include <iostream>

using namespace com::etrita::eros::cytos::module;

int main() {
  auto path = std::filesystem::current_path();

  // 创建加载器
  Loader loader({path.string()});

  // 打印模块信息
  loader.Dump();

  // 实例化所有模块
  auto all_objects = loader.InstantiateAll();

  // 使用过滤器实例化
  auto service_objects = loader.Instantiate(
      [](const std::string&, const ModuleInfo& info) {
        return info.category == "service/data_service";
      });

  // 按类别实例化
  auto by_category = loader.InstantiateByCategory("service/data_service");

  return 0;
}
```

## 编译说明

### 编译动态库

```bash
g++ -shared -fPIC -o libmy_module.so my_module.cc \
    -I/path/to/cytoskeleton/include \
    -L/path/to/cytoskeleton/lib \
    -lcytoskeleton_module
```

### Bazel 配置

```python
cc_library(
    name = "my_module",
    srcs = ["my_module.cc"],
    deps = ["//include/cytoskeleton/module"],
    linkshared = 1,
)
```

## 注意事项

1. **动态库不卸载**：加载的动态库在进程生命周期内保持加载状态
2. **线程安全**：Loader 使用并发安全的 Map 存储数据，支持多线程环境
3. **符号可见性**：使用 `extern "C"` 导出必要函数，避免 C++ 名称修饰问题
4. **跨库边界**：使用原始指针和自定义删除器管理对象生命周期
