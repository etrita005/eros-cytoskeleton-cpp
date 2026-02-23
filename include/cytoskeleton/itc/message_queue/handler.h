#pragma once

#include <concepts>
#include <functional>
#include <memory>

#include "cytoskeleton/itc/message_queue/message.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace itc {
namespace message_queue {

// Forward declaration
class Looper;

// Concept for message types
template <typename T>
concept MessageType = std::derived_from<T, Message>;

// Interface for all handlers
class IHandler {
 public:
  using Ptr = std::shared_ptr<IHandler>;

  virtual ~IHandler() = default;

  virtual void Run(Message::Ptr message) = 0;
  virtual bool IsAsync() const { return false; }
};

// Template handler class for type-safe message handling
template <MessageType T>
class Handler : public IHandler {
 public:
  using Ptr = std::shared_ptr<Handler<T>>;
  using MessageType = T;

  void Run(Message::Ptr message) override {
    Handle(std::static_pointer_cast<T>(message));
  }

 protected:
  virtual void Handle(std::shared_ptr<T> message) = 0;
};

// Lambda handler wrapper for convenient handler creation
template <MessageType T>
class LambdaHandler : public Handler<T> {
 public:
  using HandlerFunc = std::function<void(std::shared_ptr<T>)>;

  explicit LambdaHandler(HandlerFunc func, bool async = false)
      : func_(std::move(func)), async_(async) {}

 protected:
  void Handle(std::shared_ptr<T> message) override { func_(message); }

 public:
  bool IsAsync() const override { return async_; }

 private:
  HandlerFunc func_;
  bool async_;
};

// Lightweight message handler for Windows-style messages (what + wparam + lparam)
class LightweightHandler : public IHandler {
 public:
  using Ptr = std::shared_ptr<LightweightHandler>;
  using HandlerFunc = std::function<void(object::Object::Ptr, object::Object::Ptr)>;

  explicit LightweightHandler(HandlerFunc func, bool async = false)
      : func_(std::move(func)), async_(async) {}

  void Run(Message::Ptr message) override {
    func_(message->GetWParam(), message->GetLParam());
  }

  bool IsAsync() const override { return async_; }

 private:
  HandlerFunc func_;
  bool async_;
};

}  // namespace message_queue
}  // namespace itc
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
