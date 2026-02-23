#pragma once

#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

template <typename K, typename V>
class Map {
 public:
  using Ptr = std::shared_ptr<Map<K, V>>;

  Map() = default;
  ~Map() = default;

  Map(const Map&) = delete;
  Map& operator=(const Map&) = delete;
  Map(Map&&) = delete;
  Map& operator=(Map&&) = delete;

  bool Insert(const K& key, const V& value) {
    WriteLock lock(mutex_);
    auto result = data_.emplace(key, value);
    return result.second;
  }

  bool Insert(K&& key, V&& value) {
    WriteLock lock(mutex_);
    auto result = data_.emplace(std::move(key), std::move(value));
    return result.second;
  }

  bool TryGet(const K& key, V& out) const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    auto it = data_.find(key);
    if (it == data_.end()) {
      return false;
    }
    out = it->second;
    return true;
  }

  bool TryRemove(const K& key, V& out) {
    WriteLock lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) {
      return false;
    }
    out = std::move(it->second);
    data_.erase(it);
    return true;
  }

  bool Contains(const K& key) const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    return data_.find(key) != data_.end();
  }

  size_t Size() const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    return data_.size();
  }

  bool Empty() const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    return data_.empty();
  }

  void Clear() {
    WriteLock lock(mutex_);
    data_.clear();
  }

  void ForEach(
      const std::function<bool(const K&, const V&)>& callback) const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    for (const auto& [key, value] : data_) {
      if (!callback(key, value)) {
        break;
      }
    }
  }

  std::vector<std::pair<K, V>> ToVector() const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    return std::vector<std::pair<K, V>>(data_.begin(), data_.end());
  }

  std::vector<K> Keys() const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    std::vector<K> result;
    result.reserve(data_.size());
    for (const auto& [key, value] : data_) {
      result.push_back(key);
    }
    return result;
  }

  std::vector<V> Values() const {
    ReadLock lock(const_cast<ReadWriteMutex&>(mutex_));
    std::vector<V> result;
    result.reserve(data_.size());
    for (const auto& [key, value] : data_) {
      result.push_back(value);
    }
    return result;
  }

 private:
  mutable ReadWriteMutex mutex_;
  std::map<K, V> data_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
