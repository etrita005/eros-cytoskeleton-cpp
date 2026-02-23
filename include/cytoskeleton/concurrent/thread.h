#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

class Thread {
 public:
  using Ptr = std::shared_ptr<Thread>;

  explicit Thread(const std::string& name) : name_(name) {}

  Thread(const std::string& name,
         std::function<void(std::stop_token)> func)
      : name_(name), func_(std::move(func)) {}

  virtual ~Thread() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  Thread(Thread&& other) noexcept
      : name_(std::move(other.name_)),
        func_(std::move(other.func_)),
        thread_(std::move(other.thread_)),
        started_(other.started_) {
    other.started_ = false;
  }

  Thread& operator=(Thread&& other) noexcept {
    if (this != &other) {
      if (thread_.joinable()) {
        thread_.join();
      }
      name_ = std::move(other.name_);
      func_ = std::move(other.func_);
      thread_ = std::move(other.thread_);
      started_ = other.started_;
      other.started_ = false;
    }
    return *this;
  }

  void Start() {
    if (started_) return;
    started_ = true;
    if (func_) {
      thread_ = std::jthread([this](std::stop_token token) {
        func_(token);
      });
    } else {
      thread_ = std::jthread([this](std::stop_token token) {
        Run(token);
      });
    }
  }

  void Join() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  bool Join(std::chrono::milliseconds timeout) {
    if (!thread_.joinable()) {
      return true;
    }
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
      if (!thread_.joinable()) {
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
  }

  void RequestStop() {
    if (thread_.joinable()) {
      thread_.request_stop();
    }
  }

  bool ShouldStop() const {
    return thread_.get_stop_token().stop_requested();
  }

  std::string GetName() const { return name_; }

  virtual void Run(std::stop_token stop_token) {
    (void)stop_token;
  }

 private:
  std::string name_;
  std::function<void(std::stop_token)> func_;
  std::jthread thread_;
  bool started_ = false;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
