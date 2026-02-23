# Cytoskeleton Module 模块

动态模块加载与管理库，支持插件化架构和运行时动态库加载。

## 设计哲学

### 核心原则

1. **动态扩展性**：运行时加载/卸载模块，无需重新编译主程序
2. **类型安全**：基于 Object 基类的模块实例化，确保类型一致性
3. **元数据驱动**：每个模块包含完整的元数据信息（版本、作者、依赖等）
4. **跨平台支持**：支持 Linux (.so)、Windows (.dll)、macOS (.dylib)

### 架构

```
com::etrita::eros::cytos::module
├── 数据结构
│   ├── ModuleInfo      # 模块元数据
│   ├── Module          # 模块定义（含创建器/删除器）
│   └── Manifest        # 动态库清单
├── 注册机制
│   ├── DEFINE_MODULE_FACTORY  # 定义模块工厂
│   ├── REGISTER_MODULE        # 注册模块（完整版）
│   └── REGISTER_MODULE_SIMPLE # 注册模块（简化版）
└── 加载器
    └── Loader          # 动态库扫描与模块实例化
```

## 使用方式

### 1. 创建模块（动态库）

```cpp
#include "cytoskeleton/module/stub.h"
#include "cytoskeleton/object/object.h"

using namespace com::etrita::eros::cytos::object;
using namespace com::etrita::eros::cytos::module;

// 定义模块类，继承自 Object
class MyServiceModule : public Object {
 public:
  // 定义模块工厂（创建器和删除器）
  DEFINE_MODULE_FACTORY(MyServiceModule);

  MyServiceModule() {
    // 构造函数
  }

  ~MyServiceModule() {
    // 析构函数
  }

  void DoSomething() {
    // 业务逻辑
  }
};

// 注册模块（简化版）
REGISTER_MODULE_SIMPLE(MyServiceModule, "1.0.0", "service/my_service",
                       "My service module description");

// 或注册模块（完整版）
REGISTER_MODULE(MyServiceModule, "1.0.0", "service/my_service",
                "My service module description",
                "Author Name",                    // 作者
                {"DependencyModule1", "DependencyModule2"});  // 依赖
```

### 2. 编译动态库

```python
# BUILD.bazel
cc_binary(
    name = "libmy_module.so",
    srcs = ["my_module.cpp"],
    copts = ["-std=c++20", "-fPIC"],
    deps = ["//include/cytoskeleton/module"],
    linkshared = 1,
    linkstatic = 0,
)
```

### 3. 加载和使用模块

```cpp
#include "cytoskeleton/module/loader.h"

using namespace com::etrita::eros::cytos::module;
using namespace com::etrita::eros::cytos::object;

int main() {
  // 创建加载器，扫描指定目录
  std::vector<std::string> paths = {"./modules", "./plugins"};
  Loader loader(paths);

  // 查看已加载的模块
  loader.Dump();

  // 实例化所有模块
  auto all_objects = loader.InstantiateAll();

  // 按类别实例化模块
  auto service_objects = loader.InstantiateByCategory("service/my_service");

  // 使用自定义过滤器实例化
  auto filtered = loader.Instantiate(
      [](const std::string& lib_path, const ModuleInfo& info) -> bool {
        return info.version >= "1.0.0" && info.author == "Author Name";
      });

  // 从特定动态库实例化
  auto lib_objects = loader.Instantiate("libmy_module.so");

  // 使用模块对象
  for (auto& obj : all_objects) {
    // obj 是 Object::Ptr 类型
    // 可以 dynamic_cast 到具体类型
  }

  return 0;
}
```

## API 参考

### 注册宏

#### DEFINE_MODULE_FACTORY(classname)

为模块类定义工厂函数（创建器和删除器）。

**必须在模块类的 public 部分使用。**

```cpp
class MyModule : public Object {
 public:
  DEFINE_MODULE_FACTORY(MyModule);
  // ...
};
```

#### REGISTER_MODULE_SIMPLE(name, version, category, description)

简化版模块注册宏。

| 参数 | 说明 |
|------|------|
| name | 模块类名 |
| version | 版本号（如 "1.0.0"） |
| category | 类别（如 "service/data"） |
| description | 描述 |

```cpp
REGISTER_MODULE_SIMPLE(MyModule, "1.0.0", "service/data",
                       "Data processing service");
```

#### REGISTER_MODULE(name, version, category, description, author, dependencies)

完整版模块注册宏。

| 参数 | 说明 |
|------|------|
| name | 模块类名 |
| version | 版本号 |
| category | 类别 |
| description | 描述 |
| author | 作者 |
| dependencies | 依赖模块列表（vector<string>） |

```cpp
REGISTER_MODULE(MyModule, "1.0.0", "service/data",
                "Data processing service",
                "Etrita Team",
                {"BaseModule", "NetworkModule"});
```

### 数据结构

#### ModuleInfo

模块元数据结构：

```cpp
struct ModuleInfo {
  std::string name;                    // 模块名
  std::string version;                 // 版本号
  std::string category;                // 类别
  std::string description;             // 描述
  std::string author;                  // 作者
  std::vector<std::string> dependencies;  // 依赖列表
  std::unordered_map<std::string, std::string> properties;  // 扩展属性
};
```

#### Loader

模块加载器类：

```cpp
class Loader {
 public:
  // 构造函数：扫描指定路径的动态库
  explicit Loader(const std::vector<std::string>& library_paths,
                  const std::string& filter = {});

  // 实例化所有模块
  std::vector<Object::Ptr> InstantiateAll();

  // 使用过滤器实例化模块
  std::vector<Object::Ptr> Instantiate(const ModuleFilter& filter);

  // 从指定动态库实例化所有模块
  std::vector<Object::Ptr> Instantiate(const std::string& library_filename);

  // 按类别实例化模块
  std::vector<Object::Ptr> InstantiateByCategory(const std::string& category);

  // 打印已加载的模块信息
  void Dump();
};
```

### 过滤器类型

```cpp
using ModuleFilter =
    std::function<bool(const std::string& lib_path, const ModuleInfo& info)>;
```

过滤器接收动态库路径和模块信息，返回是否实例化该模块。

## 完整示例

### 示例 1：创建数据服务模块

```cpp
// data_service_module.cpp
#include "cytoskeleton/module/stub.h"
#include "cytoskeleton/object/object.h"
#include <iostream>

using namespace com::etrita::eros::cytos::object;
using namespace com::etrita::eros::cytos::module;

class DataServiceModule : public Object {
 public:
  DEFINE_MODULE_FACTORY(DataServiceModule);

  DataServiceModule() {
    std::cout << "DataServiceModule constructed" << std::endl;
  }

  ~DataServiceModule() {
    std::cout << "DataServiceModule destroyed" << std::endl;
  }

  void ProcessData() {
    std::cout << "Processing data..." << std::endl;
  }
};

REGISTER_MODULE_SIMPLE(DataServiceModule, "1.0.0", "service/data_service",
                       "Sample data service module");
```

### 示例 2：加载器程序

```cpp
// main.cpp
#include "cytoskeleton/module/loader.h"
#include <iostream>

using namespace com::etrita::eros::cytos::module;
using namespace com::etrita::eros::cytos::object;

int main() {
  // 扫描当前目录和 bazel-bin 目录
  std::vector<std::string> paths = {
    std::filesystem::current_path().string(),
    "bazel-bin/examples/module"
  };

  Loader loader(paths);
  loader.Dump();

  // 实例化所有模块
  auto objects = loader.InstantiateAll();
  std::cout << "Created " << objects.size() << " objects" << std::endl;

  return 0;
}
```

### 示例 3：BUILD 配置

```python
# 动态库模块
cc_binary(
    name = "libdata_service_module.so",
    srcs = ["data_service_module.cpp"],
    copts = ["-std=c++20", "-fPIC"],
    deps = ["//include/cytoskeleton/module"],
    linkshared = 1,
    linkstatic = 0,
)

# 加载器程序
cc_binary(
    name = "loader_app",
    srcs = ["main.cpp"],
    copts = ["-std=c++20"],
    deps = ["//include/cytoskeleton/module"],
    data = [":libdata_service_module.so"],
)
```

## 注意事项

1. **非 Header-Only**：module 模块需要编译为静态库或动态库，因为它维护全局状态（模块注册表）

2. **模块生命周期**：
   - 动态库加载后，模块元数据即被注册
   - 调用 Instantiate 方法时才创建模块实例
   - 实例生命周期由 Object::Ptr（shared_ptr）管理

3. **依赖管理**：
   - REGISTER_MODULE 可以声明依赖，但当前版本不自动解析依赖顺序
   - 建议按依赖顺序实例化模块

4. **线程安全**：
   - Loader 的实例化方法是线程安全的
   - 模块内部的线程安全由模块自身保证

## 测试

```bash
# 运行模块测试
bazel test //tests/module:all

# 运行示例
bazel run //examples/module:loader_example
```
