#pragma once

#include <boost/property_tree/ptree.hpp>
#include <functional>
#include <memory>
#include <string>

#include "cytoskeleton/concurrent/mutex.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

class Tree {
 public:
  using Ptr = std::shared_ptr<Tree>;

  Tree() = default;
  ~Tree() = default;

  Tree(const Tree&) = delete;
  Tree& operator=(const Tree&) = delete;
  Tree(Tree&&) = delete;
  Tree& operator=(Tree&&) = delete;

  void Put(const std::string& path,
           const boost::property_tree::ptree& value) {
    MutexLock lock(mutex_);
    data_.put_child(path, value);
  }

  boost::property_tree::ptree Get(const std::string& path) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    return data_.get_child(path);
  }

  bool HasPath(const std::string& path) const {
    MutexLock lock(const_cast<Mutex&>(mutex_));
    boost::property_tree::ptree::const_assoc_iterator it =
        data_.find(path);
    return it != data_.not_found();
  }

  void Remove(const std::string& path) {
    MutexLock lock(mutex_);
    data_.erase(path);
  }

  void Clear() {
    MutexLock lock(mutex_);
    data_.clear();
  }

  void ForEach(const std::function<bool(const std::string&,
                                        const boost::property_tree::ptree&)>&
                   callback) {
    MutexLock lock(mutex_);
    for (const auto& [key, value] : data_) {
      if (!callback(key, value)) {
        break;
      }
    }
  }

 private:
  mutable Mutex mutex_;
  boost::property_tree::ptree data_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
