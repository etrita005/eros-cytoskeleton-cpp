#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace concurrent {

class ThreadPool {
 public:
  using Ptr = std::shared_ptr<ThreadPool>;

  explicit ThreadPool(size_t pool_size)
      : pool_(static_cast<int>(pool_size)),
        work_guard_(boost::asio::make_work_guard(pool_)),
        stopped_(false) {}

  ~ThreadPool() { Shutdown(); }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  template <typename F, typename... Args>
  auto Submit(F&& f, Args&&... args)
      -> std::future<decltype(f(args...))> {
    using ReturnType = decltype(f(args...));

    if (stopped_.load()) {
      throw std::runtime_error("Cannot submit task to stopped ThreadPool");
    }

    auto promise = std::make_shared<std::promise<ReturnType>>();
    std::future<ReturnType> result = promise->get_future();

    boost::asio::post(pool_,
                      [promise, func = std::bind(std::forward<F>(f),
                                                  std::forward<Args>(args)...)]() mutable {
                        try {
                          if constexpr (std::is_void_v<ReturnType>) {
                            func();
                            promise->set_value();
                          } else {
                            promise->set_value(func());
                          }
                        } catch (...) {
                          promise->set_exception(std::current_exception());
                        }
                      });

    return result;
  }

  void Shutdown() {
    if (stopped_.exchange(true)) {
      return;
    }
    if (work_guard_) {
      work_guard_.reset();
    }
    pool_.join();
  }

  void Join() {
    if (stopped_.load()) {
      return;
    }
    if (work_guard_) {
      work_guard_.reset();
    }
    pool_.join();
  }

  bool IsRunning() const { return !stopped_.load(); }

 private:
  boost::asio::thread_pool pool_;
  std::optional<boost::asio::executor_work_guard<
      boost::asio::thread_pool::executor_type>>
      work_guard_;
  std::atomic<bool> stopped_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
