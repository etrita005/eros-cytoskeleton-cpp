#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "cytoskeleton/module/loader.h"
#include "cytoskeleton/module/stub.h"

using namespace com::etrita::eros::cytos::module;
using namespace com::etrita::eros::cytos::object;

class TestModuleA : public Object {
 public:
  DEFINE_MODULE_FACTORY(TestModuleA);

  TestModuleA() {}
  
  std::string GetName() const { return "TestModuleA"; }
};

REGISTER_MODULE_SIMPLE(TestModuleA, "1.0.0", "test/category_a",
                       "Test module A for unit testing");

class TestModuleB : public Object {
 public:
  DEFINE_MODULE_FACTORY(TestModuleB);

  TestModuleB() {}
  
  std::string GetName() const { return "TestModuleB"; }
};

REGISTER_MODULE(TestModuleB, "2.0.0", "test/category_b",
                "Test module B for unit testing", "TestAuthor",
                {"TestModuleA"});

TEST(ModuleTest, ModuleRegistration) {
  const char* manifest_json = GetManifest();
  ASSERT_NE(manifest_json, nullptr);
  EXPECT_GT(strlen(manifest_json), 0u);
}

TEST(ModuleTest, GetModuleCreator) {
  Object* (*creator)() = nullptr;
  void (*deleter)(Object*) = nullptr;

  GetModuleCreator("TestModuleA", &creator, &deleter);
  ASSERT_NE(creator, nullptr);
  ASSERT_NE(deleter, nullptr);

  Object* obj = creator();
  ASSERT_NE(obj, nullptr);
  deleter(obj);
}

TEST(ModuleTest, GetModuleCreatorNotFound) {
  Object* (*creator)() = nullptr;
  void (*deleter)(Object*) = nullptr;

  GetModuleCreator("NonExistentModule", &creator, &deleter);
  EXPECT_EQ(creator, nullptr);
  EXPECT_EQ(deleter, nullptr);
}

TEST(ModuleInfoTest, StructureTest) {
  ModuleInfo info;
  info.name = "TestModule";
  info.version = "1.0.0";
  info.category = "test/category";
  info.description = "Test description";
  info.author = "TestAuthor";
  info.dependencies = {"Dep1", "Dep2"};
  info.properties["key1"] = "value1";

  EXPECT_EQ(info.name, "TestModule");
  EXPECT_EQ(info.version, "1.0.0");
  EXPECT_EQ(info.category, "test/category");
  EXPECT_EQ(info.description, "Test description");
  EXPECT_EQ(info.author, "TestAuthor");
  EXPECT_EQ(info.dependencies.size(), 2u);
  EXPECT_EQ(info.properties["key1"], "value1");
}

TEST(ManifestTest, StructureTest) {
  Manifest manifest;
  manifest.library_filename = "/path/to/libtest.so";

  ModuleInfo info;
  info.name = "TestModule";
  info.version = "1.0.0";
  manifest.modules["TestModule"] = info;

  EXPECT_EQ(manifest.library_filename, "/path/to/libtest.so");
  EXPECT_EQ(manifest.modules.size(), 1u);
  EXPECT_EQ(manifest.modules["TestModule"].name, "TestModule");
}

TEST(ModuleFilterTest, FilterByCategory) {
  ModuleInfo info_a;
  info_a.name = "ModuleA";
  info_a.category = "service/data";

  ModuleInfo info_b;
  info_b.name = "ModuleB";
  info_b.category = "service/network";

  ModuleFilter filter = [](const std::string&, const ModuleInfo& info) -> bool {
    return info.category == "service/data";
  };

  EXPECT_TRUE(filter("", info_a));
  EXPECT_FALSE(filter("", info_b));
}

TEST(ModuleFilterTest, FilterByVersion) {
  ModuleInfo info;
  info.name = "TestModule";
  info.version = "2.0.0";

  ModuleFilter filter = [](const std::string&, const ModuleInfo& info) -> bool {
    return info.version >= "1.0.0";
  };

  EXPECT_TRUE(filter("", info));
}

TEST(ModuleFilterTest, FilterByProperty) {
  ModuleInfo info;
  info.name = "TestModule";
  info.properties["enabled"] = "true";

  ModuleFilter filter = [](const std::string&, const ModuleInfo& info) -> bool {
    return info.properties.count("enabled") &&
           info.properties.at("enabled") == "true";
  };

  EXPECT_TRUE(filter("", info));
}

TEST(ModuleFilterTest, EmptyFilter) {
  ModuleFilter filter;
  EXPECT_FALSE(filter);
}
