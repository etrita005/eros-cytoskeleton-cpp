#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace object {
class Object;
}  // namespace object

namespace module {

struct ModuleInfo {
  std::string name;
  std::string version;
  std::string category;
  std::string description;
  std::string author;
  std::vector<std::string> dependencies;
  std::unordered_map<std::string, std::string> properties;
};

struct Module {
  ModuleInfo info;
  object::Object* (*creator)();
  void (*deleter)(object::Object*);
};

struct Manifest {
  std::string library_filename;
  std::unordered_map<std::string, ModuleInfo> modules;
};

}  // namespace module
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
