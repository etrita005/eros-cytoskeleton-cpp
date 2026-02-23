#pragma once

#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename T>
class List {
 public:
  using Ptr = std::shared_ptr<List<T>>;

  List() = default;
  ~List() = default;

  List(const List&) = delete;
  List& operator=(const List&) = delete;
  List(List&&) = delete;
  List& operator=(List&&) = delete;

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
    data_.push_front(value);
  }

  void PushFront(T&& value) {
    MutexLock lock(mutex_);
    data_.push_front(std::move(value));
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
    data_.pop_front();
    return true;
  }

  bool TryGet(size_t index, T& out) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    if (index >= data_.size()) {
      return false;
    }
    auto it = data_.begin();
    std::advance(it, index);
    out = *it;
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
    std::vector<T> result;
    auto it = data_.begin();
    std::advance(it, start);
    for (size_t i = start; i < end; ++i, ++it) {
      result.push_back(*it);
    }
    return result;
  }

  std::vector<T> ToVector() const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    return std::vector<T>(data_.begin(), data_.end());
  }

 private:
  mutable Mutex mutex_;
  std::list<T> data_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
