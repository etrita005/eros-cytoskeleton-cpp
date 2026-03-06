#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stack>
#include <vector>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename T>
class Stack {
 public:
  using Ptr = std::shared_ptr<Stack<T>>;

  Stack() = default;
  ~Stack() = default;

  Stack(const Stack&) = delete;
  Stack& operator=(const Stack&) = delete;
  Stack(Stack&&) = delete;
  Stack& operator=(Stack&&) = delete;

  void Push(const T& value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      data_.push(value);
    }
    cv_.notify_one();
  }

  void Push(T&& value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      data_.push(std::move(value));
    }
    cv_.notify_one();
  }

  bool Pop(T& out) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !data_.empty(); });
    out = std::move(data_.top());
    data_.pop();
    return true;
  }

  bool TryPop(T& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.top());
    data_.pop();
    return true;
  }

  bool TryGet(T& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = data_.top();
    return true;
  }

  size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
  }

  bool Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.empty();
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!data_.empty()) {
      data_.pop();
    }
  }

  std::vector<T> ToVector() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<T> result;
    std::stack<T> temp = data_;
    while (!temp.empty()) {
      result.push_back(temp.top());
      temp.pop();
    }
    return result;
  }

  std::vector<T> Filter(
      const std::function<bool(const T&)>& predicate) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<T> result;
    std::stack<T> temp = data_;
    while (!temp.empty()) {
      if (predicate(temp.top())) {
        result.push_back(temp.top());
      }
      temp.pop();
    }
    return result;
  }

 private:
  mutable std::mutex mutex_;
  std::stack<T> data_;
  std::condition_variable cv_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
