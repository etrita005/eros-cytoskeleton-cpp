#include "cytoskeleton/module/stub.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <sstream>
#include <vector>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace module {

using object::Object;

static Manifest __manifest__;
static std::vector<Module> __modules__;

extern "C" void RegisterModule(
    const std::string& name, const std::string& version,
    const std::string& category, const std::string& description,
    const std::string& author, const std::vector<std::string>& dependencies,
    Object* (*creator)(), void (*deleter)(Object*)) {
  ModuleInfo info{name, version, category, description, author, dependencies, {}};
  __manifest__.modules[name] = info;
  __modules__.push_back(Module{info, creator, deleter});
}

static std::string SerializeToJson(const Manifest& manifest) {
  boost::property_tree::ptree root;
  root.put("library_filename", manifest.library_filename);

  boost::property_tree::ptree modules_node;
  for (const auto& [name, info] : manifest.modules) {
    boost::property_tree::ptree module_node;
    module_node.put("name", info.name);
    module_node.put("version", info.version);
    module_node.put("category", info.category);
    module_node.put("description", info.description);
    module_node.put("author", info.author);

    boost::property_tree::ptree deps_node;
    for (const auto& dep : info.dependencies) {
      boost::property_tree::ptree dep_node;
      dep_node.put("", dep);
      deps_node.push_back(std::make_pair("", dep_node));
    }
    module_node.add_child("dependencies", deps_node);

    boost::property_tree::ptree props_node;
    for (const auto& [key, value] : info.properties) {
      props_node.put(key, value);
    }
    module_node.add_child("properties", props_node);

    modules_node.push_back(std::make_pair(name, module_node));
  }
  root.add_child("modules", modules_node);

  std::ostringstream oss;
  boost::property_tree::write_json(oss, root);
  return oss.str();
}

extern "C" const char* GetManifest() {
  static thread_local std::string json_str;
  json_str = SerializeToJson(__manifest__);
  return json_str.c_str();
}

extern "C" void GetModuleCreator(const char* module_name,
                                  Object* (**creator)(),
                                  void (**deleter)(Object*)) {
  for (const auto& module : __modules__) {
    if (module.info.name == module_name) {
      *creator = module.creator;
      *deleter = module.deleter;
      return;
    }
  }
  *creator = nullptr;
  *deleter = nullptr;
}

}  // namespace module
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
