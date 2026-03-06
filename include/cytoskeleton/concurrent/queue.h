#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename T>
class Queue {
 public:
  using Ptr = std::shared_ptr<Queue<T>>;

  Queue() = default;
  ~Queue() = default;

  Queue(const Queue&) = delete;
  Queue& operator=(const Queue&) = delete;
  Queue(Queue&&) = delete;
  Queue& operator=(Queue&&) = delete;

  void Enqueue(const T& value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      data_.push(value);
    }
    cv_.notify_one();
  }

  void Enqueue(T&& value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      data_.push(std::move(value));
    }
    cv_.notify_one();
  }

  bool Dequeue(T& out) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !data_.empty(); });
    out = std::move(data_.front());
    data_.pop();
    return true;
  }

  bool TryDequeue(T& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = std::move(data_.front());
    data_.pop();
    return true;
  }

  bool TryGet(T& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    out = data_.front();
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
    std::queue<T> temp = data_;
    while (!temp.empty()) {
      result.push_back(temp.front());
      temp.pop();
    }
    return result;
  }

  std::vector<T> Filter(
      const std::function<bool(const T&)>& predicate) const {
    std::lock_guard<std::mutex> lock(mutex_);
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
  mutable std::mutex mutex_;
  std::queue<T> data_;
  std::condition_variable cv_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
