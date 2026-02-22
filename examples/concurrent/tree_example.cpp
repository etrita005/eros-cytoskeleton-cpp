// Example: Thread-Safe Property Tree
// Demonstrates hierarchical data storage using boost::property_tree

#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/tree.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic key-value storage
  std::cout << "=== Basic Key-Value Storage ===" << std::endl;
  {
    Tree config;

    boost::property_tree::ptree value;
    value.put("", "192.168.1.1");
    config.Put("server.ip", value);

    value.put("", "8080");
    config.Put("server.port", value);

    value.put("", "debug");
    config.Put("log.level", value);

    auto ip = config.Get("server.ip").get<std::string>("");
    auto port = config.Get("server.port").get<std::string>("");

    std::cout << "Server: " << ip << ":" << port << std::endl;
  }

  // Example 2: Complex nested structure
  std::cout << "\n=== Complex Nested Structure ===" << std::endl;
  {
    Tree database;

    // Add connection settings
    boost::property_tree::ptree conn;
    conn.put("host", "localhost");
    conn.put("port", 5432);
    conn.put("database", "myapp");
    database.Put("connection", conn);

    // Add credentials
    boost::property_tree::ptree creds;
    creds.put("username", "admin");
    creds.put("password", "secret");
    database.Put("credentials", creds);

    // Add pool settings
    boost::property_tree::ptree pool;
    pool.put("min", 5);
    pool.put("max", 20);
    database.Put("pool", pool);

    // Read back
    auto conn_info = database.Get("connection");
    std::cout << "DB Host: " << conn_info.get<std::string>("host") << std::endl;
    std::cout << "DB Port: " << conn_info.get<int>("port") << std::endl;
  }

  // Example 3: Path existence checking
  std::cout << "\n=== Path Existence Checking ===" << std::endl;
  {
    Tree settings;

    boost::property_tree::ptree val;
    val.put("", "value");
    settings.Put("feature.enabled", val);

    std::cout << "Has 'feature.enabled': "
              << (settings.HasPath("feature.enabled") ? "yes" : "no") << std::endl;
    std::cout << "Has 'feature.timeout': "
              << (settings.HasPath("feature.timeout") ? "yes" : "no") << std::endl;
  }

  // Example 4: Iteration over entries
  std::cout << "\n=== Iteration Over Entries ===" << std::endl;
  {
    Tree users;

    for (int i = 1; i <= 3; ++i) {
      boost::property_tree::ptree user;
      user.put("name", "User" + std::to_string(i));
      user.put("role", i == 1 ? "admin" : "user");
      users.Put("user" + std::to_string(i), user);
    }

    std::cout << "Users:" << std::endl;
    users.ForEach([](const std::string& key,
                     const boost::property_tree::ptree& val) {
      std::cout << "  " << key << ": " << val.get<std::string>("name")
                << " (" << val.get<std::string>("role") << ")" << std::endl;
      return true;
    });
  }

  // Example 5: Concurrent configuration updates
  std::cout << "\n=== Concurrent Configuration Updates ===" << std::endl;
  {
    Tree config;
    const int num_threads = 4;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&config, i]() {
        for (int j = 0; j < 10; ++j) {
          std::string key = "service" + std::to_string(i) + ".metric" + std::to_string(j);
          boost::property_tree::ptree val;
          val.put("", i * 100 + j);
          config.Put(key, val);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    // Count total entries
    int count = 0;
    config.ForEach([&count](const std::string&, const boost::property_tree::ptree&) {
      ++count;
      return true;
    });

    std::cout << "Total configuration entries: " << count << std::endl;
  }

  return 0;
}
