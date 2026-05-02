#include <iostream>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

bool TryConnect(int attempt) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return attempt >= 5;
}

int main() {
  std::cout << "=== Deadline 示例 ===" << std::endl;

  // 1. 超时控制
  auto deadline = Deadline::After(Duration::FromMilliseconds(500));
  int attempt = 0;

  while (!deadline.IsExpired()) {
    ++attempt;
    if (TryConnect(attempt)) {
      std::cout << "连接成功! 尝试次数: " << attempt << std::endl;
      break;
    }
    auto remaining = deadline.Remaining();
    std::cout << "尝试 " << attempt << " 失败, 剩余时间: "
              << remaining.ToMilliseconds() << "ms" << std::endl;
  }

  if (deadline.IsExpired()) {
    std::cout << "连接超时!" << std::endl;
  }

  // 2. 查看截止时间
  auto d = Deadline::After(Duration::FromSeconds(10));
  std::cout << "截止时间 (单调时钟): " << d.ExpireTime().ToChrono().time_since_epoch().count()
            << "ns" << std::endl;

  return 0;
}
