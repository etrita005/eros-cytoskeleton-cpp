#include <iostream>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

int main() {
  std::cout << "=== Duration 示例 ===" << std::endl;

  // 1. 创建 Duration
  auto d1 = Duration::FromMilliseconds(500);
  auto d2 = Duration::FromSeconds(1);
  auto d3 = Duration::FromSecondsDouble(1.5);

  std::cout << "d1 = " << d1.ToMilliseconds() << "ms" << std::endl;
  std::cout << "d2 = " << d2.ToMilliseconds() << "ms" << std::endl;
  std::cout << "d3 = " << d3.ToMilliseconds() << "ms" << std::endl;

  // 2. 算术运算
  auto total = d1 + d2;
  std::cout << "d1 + d2 = " << total.ToMilliseconds() << "ms" << std::endl;

  auto diff = d2 - d1;
  std::cout << "d2 - d1 = " << diff.ToMilliseconds() << "ms" << std::endl;

  auto doubled = d1 * 2;
  std::cout << "d1 * 2 = " << doubled.ToMilliseconds() << "ms" << std::endl;

  // 3. 比较运算
  std::cout << "d1 < d2: " << (d1 < d2 ? "true" : "false") << std::endl;

  // 4. 工具方法
  auto neg = Duration::FromMilliseconds(-100);
  std::cout << "neg.IsNegative(): " << (neg.IsNegative() ? "true" : "false") << std::endl;
  std::cout << "neg.Abs().ToMilliseconds(): " << neg.Abs().ToMilliseconds() << "ms" << std::endl;

  // 5. Duration 除法
  auto timeout = Duration::FromMilliseconds(500);
  auto interval = Duration::FromMilliseconds(50);
  std::cout << "500ms / 50ms = " << (timeout / interval) << std::endl;

  return 0;
}
