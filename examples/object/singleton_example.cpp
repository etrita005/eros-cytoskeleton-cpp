// Copyright (c) 2024 Etrita. All rights reserved.
//
// 例程名称: singleton_example
// 例程用途: 演示 Singleton 单例模式的使用，包括全局配置管理和资源池
//
// 功能说明:
//   1. 实现全局配置管理器单例
//   2. 实现资源池单例
//   3. 演示线程安全的单例访问
//   4. 演示单例生命周期管理

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/object/singleton.h"

using namespace com::etrita::eros::cytos::object;

// 全局配置管理器单例
class ConfigManager : public Singleton<ConfigManager> {
  friend class Singleton<ConfigManager>;

 public:
  void SetConfig(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    configs_[key] = value;
    std::cout << "[ConfigManager] Set " << key << " = " << value << std::endl;
  }

  std::string GetConfig(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configs_.find(key);
    if (it != configs_.end()) {
      return it->second;
    }
    return "";
  }

  void PrintAllConfigs() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[ConfigManager] All configs:" << std::endl;
    for (const auto& [key, value] : configs_) {
      std::cout << "  " << key << " = " << value << std::endl;
    }
  }

 protected:
  ConfigManager() {
    std::cout << "[ConfigManager] Constructor called" << std::endl;
    // 加载默认配置
    configs_["app_name"] = "MyApplication";
    configs_["version"] = "1.0.0";
  }

  ~ConfigManager() override {
    std::cout << "[ConfigManager] Destructor called" << std::endl;
    // 保存配置到文件
  }

 private:
  std::mutex mutex_;
  std::map<std::string, std::string> configs_;
};

// 数据库连接池单例
class ConnectionPool : public Singleton<ConnectionPool> {
  friend class Singleton<ConnectionPool>;

 public:
  void InitializePool(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_size_ = size;
    available_connections_ = size;
    std::cout << "[ConnectionPool] Initialized with " << size << " connections"
              << std::endl;
  }

  bool AcquireConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (available_connections_ > 0) {
      --available_connections_;
      std::cout << "[ConnectionPool] Connection acquired, available: "
                << available_connections_ << std::endl;
      return true;
    }
    std::cout << "[ConnectionPool] No available connections" << std::endl;
    return false;
  }

  void ReleaseConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (available_connections_ < pool_size_) {
      ++available_connections_;
      std::cout << "[ConnectionPool] Connection released, available: "
                << available_connections_ << std::endl;
    }
  }

  size_t GetAvailableConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_connections_;
  }

 protected:
  ConnectionPool() {
    std::cout << "[ConnectionPool] Constructor called" << std::endl;
  }

  ~ConnectionPool() override {
    std::cout << "[ConnectionPool] Destructor called" << std::endl;
  }

 private:
  std::mutex mutex_;
  size_t pool_size_ = 0;
  size_t available_connections_ = 0;
};

int main() {
  std::cout << "=== Singleton Example ===" << std::endl;

  // 示例 1: 基本单例使用
  std::cout << "\n[示例 1] 配置管理器单例" << std::endl;
  {
    // 获取单例实例
    auto config = ConfigManager::Instance();
    config->PrintAllConfigs();

    // 修改配置
    config->SetConfig("database_url", "localhost:5432");
    config->SetConfig("timeout", "30");

    config->PrintAllConfigs();

    // 再次获取实例，应该是同一个对象
    auto config2 = ConfigManager::Instance();
    std::cout << "Same instance: " << (config.get() == config2.get() ? "Yes" : "No")
              << std::endl;
  }

  // 示例 2: 资源池单例
  std::cout << "\n[示例 2] 连接池单例" << std::endl;
  {
    auto pool = ConnectionPool::Instance();
    pool->InitializePool(3);

    // 获取连接
    pool->AcquireConnection();
    pool->AcquireConnection();

    std::cout << "Available: " << pool->GetAvailableConnections() << std::endl;

    // 释放连接
    pool->ReleaseConnection();

    std::cout << "Available after release: " << pool->GetAvailableConnections()
              << std::endl;
  }

  // 示例 3: 多线程环境下的单例
  std::cout << "\n[示例 3] 线程安全的单例访问" << std::endl;
  {
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
      threads.emplace_back([i]() {
        auto config = ConfigManager::Instance();
        config->SetConfig("thread_" + std::to_string(i) + "_config",
                         "value_" + std::to_string(i));
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "All threads completed" << std::endl;
    ConfigManager::Instance()->PrintAllConfigs();
  }

  std::cout << "\n=== Example completed ===" << std::endl;
  return 0;
}
