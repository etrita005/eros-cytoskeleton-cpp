#pragma once

#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename T>
class Queue {
 public:
  Queue() = default;
  ~Queue() = default;

  Queue(const Queue&) = delete;
  Queue& operator=(const Queue&) = delete;
  Queue(Queue&&) = delete;
  Queue& operator=(Queue&&) = delete;

  void Enqueue(const T& value) {
    {
      MutexLock lock(mutex_);
      data_.push(value);
    }
    cv_.notify_one();
  }

  void Enqueue(T&& value) {
    {
      MutexLock lock(mutex_);
      data_.push(std::move(value));
    }
    cv_.notify_one();
  }

  bool Dequeue(T& out) {
    std::unique_lock<std::mutex> lock(cv_mutex_);
    cv_.wait(lock, [this] { return !data_.empty(); });
    out = std::move(data_.front());
    data_.pop();
    return true;
  }

  bool TryDequeue(T& out) {
    MutexLock lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.front());
    data_.pop();
    return true;
  }

  bool TryGet(T& out) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    if (data_.empty()) {
      return false;
    }
    out = data_.front();
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
    std::queue<T> temp = data_;
    while (!temp.empty()) {
      result.push_back(temp.front());
      temp.pop();
    }
    return result;
  }

  std::vector<T> Filter(
      const std::function<bool(const T&)>& predicate) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    std::vector<T> result;
    std::queue<T> temp = data_;
    while (!temp.empty()) {
      if (predicate(temp.front())) {
        result.push_back(temp.front());
      }
      temp.pop();
    }
    return result;
  }

 private:
  mutable Mutex mutex_;
  std::queue<T> data_;
  std::mutex cv_mutex_;
  std::condition_variable cv_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
