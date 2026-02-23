#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

// Platform-specific includes for setting thread name
#ifdef _WIN32
#include <windows.h>
// For SetThreadDescription (Windows 10 1607+)
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

// Cross-platform function to set thread name at OS level
inline void SetNativeThreadName(const std::string& name) {
#ifdef _WIN32
  // Windows: Try SetThreadDescription (Windows 10 1607+)
  HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
  if (kernel32) {
    auto set_thread_desc = reinterpret_cast<SetThreadDescriptionFunc>(
        GetProcAddress(kernel32, "SetThreadDescription"));
    if (set_thread_desc) {
      // Convert UTF-8 to UTF-16
      int wlen = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
      if (wlen > 0) {
        std::wstring wname(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, &wname[0], wlen);
        set_thread_desc(GetCurrentThread(), wname.c_str());
      }
    }
  }
#elif defined(__APPLE__)
  // macOS: pthread_setname_np (max 64 bytes)
  pthread_setname_np(name.c_str());
#else
  // Linux: pthread_setname_np (max 16 bytes including null)
  // Truncate to 15 characters to fit the limit
  std::string truncated = name.substr(0, 15);
  pthread_setname_np(pthread_self(), truncated.c_str());
#endif
}

}  // namespace detail

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
        detail::SetNativeThreadName(name_);
        func_(token);
      });
    } else {
      thread_ = std::jthread([this](std::stop_token token) {
        detail::SetNativeThreadName(name_);
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
