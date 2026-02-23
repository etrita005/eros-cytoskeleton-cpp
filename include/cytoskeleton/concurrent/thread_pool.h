#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
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

  explicit ThreadPool(size_t pool_size) : stop_(false) {
    for (size_t i = 0; i < pool_size; ++i) {
      workers_.emplace_back([this](std::stop_token stop_token) {
        WorkerLoop(stop_token);
      });
    }
  }

  ~ThreadPool() { Shutdown(); }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  template <typename F, typename... Args>
  auto Submit(F&& f, Args&&... args)
      -> std::future<decltype(f(args...))> {
    using ReturnType = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<ReturnType> result = task->get_future();

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (stop_) {
        throw std::runtime_error("Cannot submit task to stopped ThreadPool");
      }
      tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return result;
  }

  void Shutdown() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      stop_ = true;
    }

    condition_.notify_all();

    for (std::jthread& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

 private:
  void WorkerLoop(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
      std::function<void()> task;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        condition_.wait(lock, [this, &stop_token] {
          return stop_ || !tasks_.empty() || stop_token.stop_requested();
        });

        if ((stop_ && tasks_.empty()) || stop_token.stop_requested()) {
          return;
        }

        task = std::move(tasks_.front());
        tasks_.pop();
      }

      task();
    }
  }

  std::vector<std::jthread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  bool stop_;
};

}  // namespace concurrent
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
