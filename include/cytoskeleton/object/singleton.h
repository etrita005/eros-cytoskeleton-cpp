#pragma once

#include <memory>
#include <mutex>

#include "cytoskeleton/object/object.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace object {

template <typename T>
class Singleton : public Object {
 public:
  using Ptr = std::shared_ptr<T>;

  static std::shared_ptr<T> Instance() {
    std::call_once(init_flag_, []() {
      instance_ = std::shared_ptr<T>(new T(), [](T* p) { Delete(p); });
    });
    return instance_;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton(Singleton&&) = delete;
  Singleton& operator=(Singleton&&) = delete;

 protected:
  Singleton() = default;
  ~Singleton() override = default;

 private:
  static void Delete(T* p) { delete p; }

  static std::shared_ptr<T> instance_;
  static std::once_flag init_flag_;
};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ = nullptr;

template <typename T>
std::once_flag Singleton<T>::init_flag_;

}  // namespace object
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
