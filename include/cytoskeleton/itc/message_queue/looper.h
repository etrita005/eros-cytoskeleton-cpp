#pragma once

#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "cytoskeleton/concurrent/thread_pool.h"
#include "cytoskeleton/itc/message_queue/handler.h"
#include "cytoskeleton/itc/message_queue/message_queue.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace itc {
namespace message_queue {

// Concept for message construction (defined here to avoid redefinition)
template <typename T, typename... Args>
concept MessageConstructible = std::derived_from<T, Message> && std::constructible_from<T, Args...>;

// Internal message type for lambda handlers (PostHandler)
// Stores the lambda function directly in the message
class LambdaHandlerMessage : public Message {
 public:
  explicit LambdaHandlerMessage(std::function<void()> func)
      : func_(std::move(func)) {}

  void Execute() {
    if (func_) {
      func_();
    }
  }

 private:
  std::function<void()> func_;
};

class Looper : public std::enable_shared_from_this<Looper> {
 public:
  using Ptr = std::shared_ptr<Looper>;
  using HandlerId = uint64_t;

   static Ptr GetMainLooper(bool auto_start = true) {
    std::lock_guard<std::mutex> lock(GetMainLooperMutex());
    if (!GetMainLooperInstance()) {
      GetMainLooperInstance() = std::make_shared<Looper>("MainLooper", auto_start);
    }
    return GetMainLooperInstance();
  }

  static void StopMainLooper() {
    std::lock_guard<std::mutex> lock(GetMainLooperMutex());
    if (GetMainLooperInstance()) {
      GetMainLooperInstance()->Exit();
      GetMainLooperInstance().reset();
    }
  }

  explicit Looper(const std::string& name = "UNKNOWN") : name_(name) {
    thread_pool_ = std::make_shared<concurrent::ThreadPool>(
        std::thread::hardware_concurrency());
  }

  Looper(const std::string& name, bool auto_start) : Looper(name) {
    if (auto_start) {
      AsyncLoop();
    }
  }

  ~Looper() { Exit(); }

  Looper(const Looper&) = delete;
  Looper& operator=(const Looper&) = delete;
  Looper(Looper&&) = delete;
  Looper& operator=(Looper&&) = delete;

  // Register class handler
  template <MessageType MessageType>
  HandlerId RegisterHandler(typename Handler<MessageType>::Ptr handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    HandlerId id = next_handler_id_++;
    auto& handler_list = handlers_[typeid(MessageType)];
    handler_list.push_back({id, handler, handler->IsAsync()});
    return id;
  }

  // Register lambda handler
  template <MessageType MessageType>
  HandlerId RegisterHandler(
      std::function<void(std::shared_ptr<MessageType>)> func,
      bool async = false) {
    auto handler = std::make_shared<LambdaHandler<MessageType>>(func, async);
    return RegisterHandler<MessageType>(handler);
  }

  // Register lightweight message handler (by what value)
  HandlerId RegisterHandler(
      int what,
      std::function<void(object::Object::Ptr, object::Object::Ptr)> func,
      bool async = false) {
    std::lock_guard<std::mutex> lock(lightweight_handlers_mutex_);
    HandlerId id = next_handler_id_++;
    auto handler = std::make_shared<LightweightHandler>(func, async);
    lightweight_handlers_[what].push_back({id, handler, async});
    return id;
  }

  // Unregister handler by type and id
  template <MessageType MessageType>
  void UnregisterHandler(HandlerId id) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    auto it = handlers_.find(typeid(MessageType));
    if (it != handlers_.end()) {
      auto& handler_list = it->second;
      handler_list.erase(
          std::remove_if(handler_list.begin(), handler_list.end(),
                         [id](const HandlerEntry& entry) { return entry.id == id; }),
          handler_list.end());
    }
  }

  // Unregister lightweight handler by what and id
  void UnregisterHandler(int what, HandlerId id) {
    std::lock_guard<std::mutex> lock(lightweight_handlers_mutex_);
    auto it = lightweight_handlers_.find(what);
    if (it != lightweight_handlers_.end()) {
      auto& handler_list = it->second;
      handler_list.erase(
          std::remove_if(handler_list.begin(), handler_list.end(),
                         [id](const HandlerEntry& entry) { return entry.id == id; }),
          handler_list.end());
    }
  }

  // Unregister all handlers for a message type
  template <MessageType MessageType>
  void UnregisterAllHandlers() {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_.erase(typeid(MessageType));
  }

  // Unregister all lightweight handlers for a what value
  void UnregisterAllHandlers(int what) {
    std::lock_guard<std::mutex> lock(lightweight_handlers_mutex_);
    lightweight_handlers_.erase(what);
  }

  // Post message (async) with automatic construction
  template <MessageType MessageType, typename... Args>
    requires MessageConstructible<MessageType, Args...>
  void Post(Args&&... args) {
    auto message = std::make_shared<MessageType>(std::forward<Args>(args)...);
    Post<MessageType>(message);
  }

  // Post message (async) with existing message
  template <MessageType MessageType>
  void Post(std::shared_ptr<MessageType> message) {
    message_queue_.EnqueueMessage(message, 0);
  }

  // Post delayed message with automatic construction
  template <MessageType MessageType, typename... Args>
    requires MessageConstructible<MessageType, Args...>
  void PostDelayed(std::chrono::milliseconds delay, Args&&... args) {
    auto message = std::make_shared<MessageType>(std::forward<Args>(args)...);
    PostDelayed<MessageType>(delay, message);
  }

  // Post delayed message with existing message
  template <MessageType MessageType>
  void PostDelayed(std::chrono::milliseconds delay,
                   std::shared_ptr<MessageType> message) {
    message_queue_.EnqueueMessage(message,
                                  static_cast<uint64_t>(delay.count()));
  }

  // Invoke message (sync) with automatic construction - C# style
  template <MessageType MessageType, typename... Args>
    requires MessageConstructible<MessageType, Args...>
  bool Invoke(Args&&... args) {
    auto message = std::make_shared<MessageType>(std::forward<Args>(args)...);
    return Invoke<MessageType>(message);
  }

  // Invoke message (sync) with existing message - C# style
  template <MessageType MessageType>
  bool Invoke(std::shared_ptr<MessageType> message) {
    if (!message_queue_.EnqueueMessage(message, 0)) {
      return false;
    }
    message->Join();
    return true;
  }

  // Invoke message with timeout - C# style
  template <MessageType MessageType, typename... Args>
    requires MessageConstructible<MessageType, Args...>
  bool InvokeWithTimeout(std::chrono::milliseconds timeout, Args&&... args) {
    auto message = std::make_shared<MessageType>(std::forward<Args>(args)...);
    return InvokeWithTimeout<MessageType>(timeout, message);
  }

  // Invoke message with timeout (existing message) - C# style
  template <MessageType MessageType>
  bool InvokeWithTimeout(std::chrono::milliseconds timeout,
                       std::shared_ptr<MessageType> message) {
    if (!message_queue_.EnqueueMessage(message, 0)) {
      return false;
    }
    return message->Join(timeout);
  }

  // Post lightweight message (by what)
  void Post(int what, object::Object::Ptr wparam = nullptr,
            object::Object::Ptr lparam = nullptr) {
    auto message = std::make_shared<Message>(what, std::move(wparam),
                                             std::move(lparam));
    message_queue_.EnqueueMessage(message, 0);
  }

  // Invoke lightweight message (sync) - C# style
  bool Invoke(int what, object::Object::Ptr wparam = nullptr,
            object::Object::Ptr lparam = nullptr) {
    auto message = std::make_shared<Message>(what, std::move(wparam),
                                             std::move(lparam));
    if (!message_queue_.EnqueueMessage(message, 0)) {
      return false;
    }
    message->Join();
    return true;
  }

  // Post a lambda handler to be executed on the looper thread
  // @param func: The lambda function to execute (void return, no parameters)
  // @param delay: Optional delay before execution (default: 0ms)
  void PostHandler(std::function<void()> func,
                   std::chrono::milliseconds delay = std::chrono::milliseconds(0)) {
    // Store the lambda directly in the message
    auto message = std::make_shared<LambdaHandlerMessage>(std::move(func));
    message_queue_.EnqueueMessage(message, static_cast<uint64_t>(delay.count()));
  }

  // Invoke a lambda handler synchronously on the looper thread
  // @param func: The lambda function to execute (void return, no parameters)
  // @return true if successful, false if looper is quitting
  bool InvokeHandler(std::function<void()> func) {
    // Store the lambda directly in the message
    auto message = std::make_shared<LambdaHandlerMessage>(std::move(func));
    if (!message_queue_.EnqueueMessage(message, 0)) {
      return false;
    }
    message->Join();
    return true;
  }

  // Invoke a lambda handler synchronously with timeout
  // @param func: The lambda function to execute (void return, no parameters)
  // @param timeout: Maximum time to wait for execution
  // @return true if successful and executed within timeout, false otherwise
  bool InvokeHandler(std::function<void()> func,
                     std::chrono::milliseconds timeout) {
    // Store the lambda directly in the message
    auto message = std::make_shared<LambdaHandlerMessage>(std::move(func));
    if (!message_queue_.EnqueueMessage(message, 0)) {
      return false;
    }
    return message->Join(timeout);
  }

  // Start message loop in a new thread
  void AsyncLoop() {
    if (running_.exchange(true)) {
      return;  // Already running
    }
    looper_thread_ = std::thread([this]() { Loop(); });
  }

  // Message loop (blocking)
  void Loop() {
    running_ = true;
    while (!quit_) {
      auto message = message_queue_.Next();
      if (message == nullptr) {
        break;
      }
      DispatchMessage(message);
      message->Notify();
    }
    running_ = false;
  }

  // Exit the looper
  void Exit() {
    quit_ = true;
    message_queue_.Quit();
    if (looper_thread_.joinable()) {
      looper_thread_.join();
    }
    if (thread_pool_) {
      thread_pool_->Shutdown();
    }
  }

  bool IsRunning() const { return running_.load(); }

  const std::string& GetName() const { return name_; }

 private:
  struct HandlerEntry {
    HandlerId id;
    IHandler::Ptr handler;
    bool async;
  };

  void DispatchMessage(Message::Ptr message) {
    // Check if quit_ is set before dispatching
    if (quit_) {
      return;
    }

    // First, try to dispatch to typed handlers
    {
      std::lock_guard<std::mutex> lock(handlers_mutex_);
      if (quit_) return;  // Double-check after acquiring lock
      auto it = handlers_.find(typeid(*message));
      if (it != handlers_.end()) {
        for (const auto& entry : it->second) {
          if (quit_) break;  // Stop dispatching if quit requested
          if (entry.async) {
            thread_pool_->Submit([entry, message]() { 
              entry.handler->Run(message); 
            });
          } else {
            entry.handler->Run(message);
          }
        }
      }
    }

    // Then, try to dispatch to lightweight handlers (by what)
    {
      std::lock_guard<std::mutex> lock(lightweight_handlers_mutex_);
      if (quit_) return;  // Double-check after acquiring lock
      auto it = lightweight_handlers_.find(message->GetWhat());
      if (it != lightweight_handlers_.end()) {
        for (const auto& entry : it->second) {
          if (quit_) break;  // Stop dispatching if quit requested
          if (entry.async) {
            thread_pool_->Submit([entry, message]() { 
              entry.handler->Run(message); 
            });
          } else {
            entry.handler->Run(message);
          }
        }
      }
    }

    // Finally, handle lambda handlers (PostHandler)
    // Check if it's a LambdaHandlerMessage and execute the stored lambda directly
    auto lambda_msg = std::dynamic_pointer_cast<LambdaHandlerMessage>(message);
    if (lambda_msg) {
      lambda_msg->Execute();
    }
  }

  static std::mutex& GetMainLooperMutex() {
    static std::mutex mutex;
    return mutex;
  }

  static Ptr& GetMainLooperInstance() {
    static Ptr instance;
    return instance;
  }

 private:
  std::string name_;
  MessageQueue message_queue_;
  std::shared_ptr<concurrent::ThreadPool> thread_pool_;
  std::thread looper_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> quit_{false};

  std::mutex handlers_mutex_;
  std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;

  std::mutex lightweight_handlers_mutex_;
  std::unordered_map<int, std::vector<HandlerEntry>> lightweight_handlers_;

  std::atomic<HandlerId> next_handler_id_{1};
};

}  // namespace message_queue
}  // namespace itc
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
