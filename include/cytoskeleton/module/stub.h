#pragma once

#include <string>
#include <vector>

#include "cytoskeleton/module/manifest.h"
#include "cytoskeleton/object/object.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace module {

#ifndef INITIALIZE_REGISTER_MODULE_PRIORITY
#define INITIALIZE_REGISTER_MODULE_PRIORITY 101
#endif

#define DEFINE_MODULE_FACTORY(classname)                                   \
  static ::com::etrita::eros::cytos::object::Object* Create() {            \
    return new classname();                                                \
  }                                                                        \
  static void Deleter(::com::etrita::eros::cytos::object::Object* obj) {   \
    delete static_cast<classname*>(obj);                                   \
  }

#ifdef _MSC_VER
#define REGISTER_MODULE(name, version, category, description, author,      \
                        dependencies)                                      \
  static void REGISTER_MODULE_##name(void);                                \
  namespace {                                                              \
    struct REGISTER_MODULE_##name##_Struct {                               \
      REGISTER_MODULE_##name##_Struct() { REGISTER_MODULE_##name(); }      \
    };                                                                     \
    static REGISTER_MODULE_##name##_Struct REGISTER_MODULE_##name##_Instance; \
  }                                                                        \
  static void REGISTER_MODULE_##name(void) {                               \
    ::com::etrita::eros::cytos::module::RegisterModule(                    \
        #name, version, category, description, author, dependencies,       \
        name::Create, name::Deleter);                                      \
  }
#else
#define REGISTER_MODULE(name, version, category, description, author,      \
                        dependencies)                                      \
  static void REGISTER_MODULE_##name(void) __attribute__((                 \
      constructor(INITIALIZE_REGISTER_MODULE_PRIORITY)));                  \
  static void REGISTER_MODULE_##name(void) {                               \
    ::com::etrita::eros::cytos::module::RegisterModule(                    \
        #name, version, category, description, author, dependencies,       \
        name::Create, name::Deleter);                                      \
  }
#endif

#define REGISTER_MODULE_SIMPLE(name, version, category, description)       \
  REGISTER_MODULE(name, version, category, description, "", {})

extern "C" void RegisterModule(
    const std::string& name, const std::string& version,
    const std::string& category, const std::string& description,
    const std::string& author, const std::vector<std::string>& dependencies,
    object::Object* (*creator)(), void (*deleter)(object::Object*));

extern "C" const char* GetManifest();

extern "C" void GetModuleCreator(const char* module_name,
                                  object::Object* (**creator)(),
                                  void (**deleter)(object::Object*));

}  // namespace module
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
