#include <iostream>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

int main() {
  std::cout << "=== WallTime 示例 ===" << std::endl;

  // 1. 获取当前时间
  auto now = WallTime::Now();
  std::cout << "当前时间: " << now.Format() << std::endl;
  std::cout << "Unix 毫秒: " << now.ToMilliseconds() << std::endl;
  std::cout << "自定义格式: " << now.Format("%Y-%m-%d %H:%M:%S") << std::endl;

  // 2. 从时间戳构造
  auto from_sec = WallTime::FromSeconds(1700000000);
  std::cout << "从秒构造: " << from_sec.Format() << std::endl;

  // 3. 时间偏移
  auto one_hour_later = now + Duration::FromSeconds(3600);
  std::cout << "一小时后: " << one_hour_later.Format() << std::endl;

  auto one_hour_ago = now - Duration::FromSeconds(3600);
  std::cout << "一小时前: " << one_hour_ago.Format() << std::endl;

  // 4. 时间差
  auto t1 = WallTime::FromSeconds(1700000000);
  auto t2 = WallTime::FromSeconds(1700003600);
  auto diff = t2 - t1;
  std::cout << "时间差: " << diff.ToSeconds() << "s" << std::endl;

  return 0;
}
