#include <filesystem>
#include <iostream>

#include "cytoskeleton/module/loader.h"

using namespace com::etrita::eros::cytos::module;
using namespace com::etrita::eros::cytos::object;

int main(int argc, char** argv) {
  std::vector<std::string> paths;
  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      paths.push_back(argv[i]);
    }
  } else {
    // 默认使用当前目录和bazel-bin目录
    paths.push_back(std::filesystem::current_path().string());
    paths.push_back("bazel-bin/examples/module");
  }

  std::cout << "=== Module Loader Example ===" << std::endl;
  std::cout << "Scanning directories:" << std::endl;
  for (const auto& path : paths) {
    std::cout << "  - " << path << std::endl;
  }

  // 创建加载器，扫描目录
  Loader loader(paths);

  std::cout << "\n=== Loaded Modules ===" << std::endl;
  loader.Dump();

  std::cout << "\n=== Instantiating All Modules ===" << std::endl;
  auto all_objects = loader.InstantiateAll();
  std::cout << "Created " << all_objects.size() << " objects" << std::endl;

  std::cout << "\n=== Instantiating by Category (service/data_service) ===" << std::endl;
  auto data_service_objects = loader.InstantiateByCategory("service/data_service");
  std::cout << "Created " << data_service_objects.size() << " data service objects" << std::endl;

  std::cout << "\n=== Instantiating with Custom Filter ===" << std::endl;
  auto filtered_objects = loader.Instantiate(
      [](const std::string& lib_path, const ModuleInfo& info) -> bool {
        std::cout << "  Checking: " << info.name << " (version: " << info.version << ")" << std::endl;
        return info.version >= "1.0.0";
      });
  std::cout << "Created " << filtered_objects.size() << " filtered objects" << std::endl;

  std::cout << "\n=== Example Complete ===" << std::endl;
  std::cout << "Objects will be destroyed when they go out of scope..." << std::endl;

  return 0;
}
