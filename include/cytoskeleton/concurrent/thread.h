#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
using SetThreadDescriptionFunc = HRESULT(WINAPI*)(HANDLE, PCWSTR);
#else
#include <pthread.h>
#endif

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

namespace detail {

inline void SetNativeThreadName(const std::string& name) {
#ifdef _WIN32
  HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
  if (kernel32) {
    auto set_thread_desc = reinterpret_cast<SetThreadDescriptionFunc>(
        GetProcAddress(kernel32, "SetThreadDescription"));
    if (set_thread_desc) {
      int wlen = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
      if (wlen > 0) {
        std::wstring wname(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, &wname[0], wlen);
        set_thread_desc(GetCurrentThread(), wname.c_str());
      }
    }
  }
#elif defined(__APPLE__)
  pthread_setname_np(name.c_str());
#else
  std::string truncated = name.substr(0, 15);
  pthread_setname_np(pthread_self(), truncated.c_str());
#endif
}

}  // namespace detail

class Thread {
 public:
  using Ptr = std::shared_ptr<Thread>;

  explicit Thread(const std::string& name)
      : name_(name),
        func_([](std::stop_token) {}) {}

  Thread(const std::string& name,
         std::function<void(std::stop_token)> func)
      : name_(name), func_(std::move(func)) {}

  ~Thread() {
    RequestStop();
    Join();
  }

  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  Thread(Thread&& other) noexcept
      : name_(std::move(other.name_)),
        func_(std::move(other.func_)),
        thread_(std::move(other.thread_)),
        started_(other.started_.load()) {
    other.started_.store(false);
  }

  Thread& operator=(Thread&& other) noexcept {
    if (this != &other) {
      RequestStop();
      Join();
      name_ = std::move(other.name_);
      func_ = std::move(other.func_);
      thread_ = std::move(other.thread_);
      started_.store(other.started_.load());
      other.started_.store(false);
    }
    return *this;
  }

  void Start() {
    bool expected = false;
    if (!started_.compare_exchange_strong(expected, true)) {
      return;
    }
    std::string name = name_;
    auto func = func_;
    thread_ = std::jthread([name, func](std::stop_token token) {
      detail::SetNativeThreadName(name);
      func(token);
    });
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

 private:
  std::string name_;
  std::function<void(std::stop_token)> func_;
  std::jthread thread_;
  std::atomic<bool> started_{false};
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
