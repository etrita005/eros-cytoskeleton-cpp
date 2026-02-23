#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <type_traits>

#include "cytoskeleton/concurrent/event.h"
#include "cytoskeleton/object/object.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace itc {
namespace message_queue {

// Forward declaration
class Looper;

class Message : public std::enable_shared_from_this<Message> {
 public:
  using Ptr = std::shared_ptr<Message>;

  Message() = default;
  explicit Message(int what) : what_(what) {}
  Message(int what, object::Object::Ptr wparam, object::Object::Ptr lparam)
      : what_(what), wparam_(std::move(wparam)), lparam_(std::move(lparam)) {}

  virtual ~Message() = default;

  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;
  Message(Message&&) = delete;
  Message& operator=(Message&&) = delete;

  int GetWhat() const { return what_; }
  void SetWhat(int what) { what_ = what; }

  object::Object::Ptr GetWParam() const { return wparam_; }
  void SetWParam(object::Object::Ptr wparam) { wparam_ = std::move(wparam); }

  object::Object::Ptr GetLParam() const { return lparam_; }
  void SetLParam(object::Object::Ptr lparam) { lparam_ = std::move(lparam); }

  uint64_t GetWhen() const { return when_; }
  void SetWhen(uint64_t when) { when_ = when; }

  void Join() { event_.Join(); }
  bool Join(std::chrono::milliseconds timeout) { return event_.Join(timeout); }
  void Notify() { event_.Notify(); }

 protected:
  int what_{0};
  object::Object::Ptr wparam_;
  object::Object::Ptr lparam_;
  uint64_t when_{0};  // Execution time in milliseconds (steady_clock)
  concurrent::ManualResetEvent event_;
};

}  // namespace message_queue
}  // namespace itc
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
