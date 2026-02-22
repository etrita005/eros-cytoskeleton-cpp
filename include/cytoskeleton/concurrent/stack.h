#pragma once

#include <functional>
#include <stack>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename T>
class Stack {
 public:
  Stack() = default;
  ~Stack() = default;

  Stack(const Stack&) = delete;
  Stack& operator=(const Stack&) = delete;
  Stack(Stack&&) = delete;
  Stack& operator=(Stack&&) = delete;

  void Push(const T& value) {
    MutexLock lock(mutex_);
    data_.push(value);
  }

  void Push(T&& value) {
    MutexLock lock(mutex_);
    data_.push(std::move(value));
  }

  bool Pop(T& out) {
    MutexLock lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.top());
    data_.pop();
    return true;
  }

  bool TryPop(T& out) {
    MutexLock lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.top());
    data_.pop();
    return true;
  }

  bool TryGet(T& out) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    if (data_.empty()) {
      return false;
    }
    out = data_.top();
    return true;
  }

  size_t Size() const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    return data_.size();
  }

  bool Empty() const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    return data_.empty();
  }

  void Clear() {
    MutexLock lock(mutex_);
    while (!data_.empty()) {
      data_.pop();
    }
  }

  std::vector<T> ToVector() const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
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
    MutexLock lock(const_cast<Mutex&>(mutex_));
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
  mutable Mutex mutex_;
  std::stack<T> data_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
