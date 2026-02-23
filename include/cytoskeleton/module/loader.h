#pragma once

#include <dlfcn.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "cytoskeleton/concurrent/map.h"
#include "cytoskeleton/module/manifest.h"
#include "cytoskeleton/object/object.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace module {

using object::Object;

using ModuleFilter =
    std::function<bool(const std::string&, const ModuleInfo&)>;

using GetManifestFunc = const char* (*)();
using GetModuleCreatorFunc = void (*)(const char*, Object*(**)(),
                                       void (**)(Object*));

struct LoadedLibrary {
  void* handle;
  GetManifestFunc get_manifest;
  GetModuleCreatorFunc get_module_creator;
  Manifest manifest;
};

class Loader {
 public:
  explicit Loader(const std::vector<std::string>& library_paths,
                  const std::string& filter = {}) {
    for (const auto& path : library_paths) {
      ScanDirectory(path, filter);
    }
  }

  virtual ~Loader() = default;

  Loader(const Loader&) = delete;
  Loader& operator=(const Loader&) = delete;
  Loader(Loader&&) = delete;
  Loader& operator=(Loader&&) = delete;

  virtual std::vector<Object::Ptr> InstantiateAll() { return Instantiate(); }

  virtual std::vector<Object::Ptr> Instantiate(
      const ModuleFilter& filter = {}) {
    std::vector<Object::Ptr> results;
    loaded_libs_.ForEach(
        [&](const std::string&, const LoadedLibrary& lib) -> bool {
          for (const auto& [name, info] : lib.manifest.modules) {
            if (!filter || filter(lib.manifest.library_filename, info)) {
              if (auto obj = InstantiateModule(lib, name)) {
                results.push_back(obj);
              }
            }
          }
          return true;
        });
    return results;
  }

  virtual std::vector<Object::Ptr> Instantiate(
      const std::string& library_filename) {
    std::vector<Object::Ptr> results;
    LoadedLibrary lib;
    if (loaded_libs_.TryGet(library_filename, lib)) {
      for (const auto& [name, info] : lib.manifest.modules) {
        (void)info;
        if (auto obj = InstantiateModule(lib, name)) {
          results.push_back(obj);
        }
      }
    }
    return results;
  }

  virtual std::vector<Object::Ptr> InstantiateByCategory(
      const std::string& category) {
    return Instantiate(
        [&category](const std::string&, const ModuleInfo& info) -> bool {
          return info.category == category;
        });
  }

  virtual Object::Ptr InstantiateByName(const std::string& library_filename,
                                        const std::string& module_name) {
    LoadedLibrary lib;
    if (loaded_libs_.TryGet(library_filename, lib)) {
      auto it = lib.manifest.modules.find(module_name);
      if (it != lib.manifest.modules.end()) {
        return InstantiateModule(lib, module_name);
      }
    }
    return nullptr;
  }

  virtual void Dump() {
    std::cout << "Loaded Libraries:" << std::endl;
    loaded_libs_.ForEach([](const std::string& filename,
                            const LoadedLibrary& lib) -> bool {
      std::cout << "  Library: " << filename << std::endl;
      for (const auto& [name, info] : lib.manifest.modules) {
        std::cout << "    Module: " << name << std::endl;
        std::cout << "      Version: " << info.version << std::endl;
        std::cout << "      Category: " << info.category << std::endl;
        std::cout << "      Description: " << info.description << std::endl;
        std::cout << "      Author: " << info.author << std::endl;
      }
      return true;
    });
  }

 private:
  void ScanDirectory(const std::string& path, const std::string& filter) {
    if (!std::filesystem::exists(path)) {
      return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      if (entry.is_regular_file()) {
        auto filename = entry.path().filename().string();
        if (filename.find(".so") != std::string::npos) {
          if (filter.empty() || filename.find(filter) != std::string::npos) {
            LoadLibrary(entry.path().string());
          }
        }
      }
    }
  }

  void LoadLibrary(const std::string& library_path) {
    if (loaded_libs_.Contains(library_path)) {
      return;
    }

    void* handle = dlopen(library_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
      std::cerr << "Failed to load library: " << library_path << std::endl;
      std::cerr << "Error: " << dlerror() << std::endl;
      return;
    }

    auto* get_manifest = reinterpret_cast<GetManifestFunc>(
        dlsym(handle, "GetManifest"));
    if (!get_manifest) {
      std::cerr << "Failed to find GetManifest in: " << library_path
                << std::endl;
      dlclose(handle);
      return;
    }

    auto* get_module_creator = reinterpret_cast<GetModuleCreatorFunc>(
        dlsym(handle, "GetModuleCreator"));
    if (!get_module_creator) {
      std::cerr << "Failed to find GetModuleCreator in: " << library_path
                << std::endl;
      dlclose(handle);
      return;
    }

    const char* manifest_json = get_manifest();
    Manifest manifest = ParseManifest(manifest_json);
    manifest.library_filename = library_path;

    LoadedLibrary lib{handle, get_manifest, get_module_creator, manifest};
    loaded_libs_.Insert(library_path, lib);
  }

  Manifest ParseManifest(const char* json_str) {
    Manifest manifest;
    try {
      boost::property_tree::ptree root;
      std::istringstream iss(json_str);
      boost::property_tree::read_json(iss, root);

      manifest.library_filename = root.get<std::string>("library_filename", "");

      const auto& modules_node = root.get_child("modules");
      for (const auto& [name, module_node] : modules_node) {
        ModuleInfo info;
        info.name = module_node.get<std::string>("name", "");
        info.version = module_node.get<std::string>("version", "");
        info.category = module_node.get<std::string>("category", "");
        info.description = module_node.get<std::string>("description", "");
        info.author = module_node.get<std::string>("author", "");

        if (module_node.count("dependencies")) {
          for (const auto& dep : module_node.get_child("dependencies")) {
            info.dependencies.push_back(dep.second.get_value<std::string>());
          }
        }

        if (module_node.count("properties")) {
          for (const auto& prop : module_node.get_child("properties")) {
            info.properties[prop.first] = prop.second.get_value<std::string>();
          }
        }

        manifest.modules[name] = info;
      }
    } catch (const std::exception& e) {
      std::cerr << "Failed to parse manifest: " << e.what() << std::endl;
    }
    return manifest;
  }

  Object::Ptr InstantiateModule(const LoadedLibrary& lib,
                                const std::string& module_name) {
    Object* (*creator)() = nullptr;
    void (*deleter)(Object*) = nullptr;

    lib.get_module_creator(module_name.c_str(), &creator, &deleter);

    if (creator && deleter) {
      Object* raw_ptr = creator();
      return Object::Ptr(raw_ptr, deleter);
    }
    return nullptr;
  }

  concurrent::Map<std::string, LoadedLibrary> loaded_libs_;
};

}  // namespace module
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
