#pragma once

#include <functional>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename T>
class Vector {
 public:
  Vector() = default;
  ~Vector() = default;

  Vector(const Vector&) = delete;
  Vector& operator=(const Vector&) = delete;
  Vector(Vector&&) = delete;
  Vector& operator=(Vector&&) = delete;

  void PushBack(const T& value) {
    MutexLock lock(mutex_);
    data_.push_back(value);
  }

  void PushBack(T&& value) {
    MutexLock lock(mutex_);
    data_.push_back(std::move(value));
  }

  void PushFront(const T& value) {
    MutexLock lock(mutex_);
    data_.insert(data_.begin(), value);
  }

  void PushFront(T&& value) {
    MutexLock lock(mutex_);
    data_.insert(data_.begin(), std::move(value));
  }

  bool PopBack(T& out) {
    MutexLock lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.back());
    data_.pop_back();
    return true;
  }

  bool PopFront(T& out) {
    MutexLock lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.front());
    data_.erase(data_.begin());
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
    data_.clear();
  }

  bool TryGet(size_t index, T& out) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    if (index >= data_.size()) {
      return false;
    }
    out = data_[index];
    return true;
  }

  T operator[](size_t index) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    return data_[index];
  }

  void ForEach(const std::function<bool(const T&)>& callback) {
    MutexLock lock(mutex_);
    for (const auto& item : data_) {
      if (!callback(item)) {
        break;
      }
    }
  }

  std::vector<T> Filter(
      const std::function<bool(const T&)>& predicate) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    std::vector<T> result;
    for (const auto& item : data_) {
      if (predicate(item)) {
        result.push_back(item);
      }
    }
    return result;
  }

  std::vector<T> Slice(size_t start, size_t end) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    if (start >= data_.size()) {
      return {};
    }
    if (end > data_.size()) {
      end = data_.size();
    }
    if (start >= end) {
      return {};
    }
    return std::vector<T>(data_.begin() + start, data_.begin() + end);
  }

 private:
  mutable Mutex mutex_;
  std::vector<T> data_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
