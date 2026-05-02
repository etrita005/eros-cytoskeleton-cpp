#include <iostream>
#include <thread>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

void DoWork() {
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

int main() {
  std::cout << "=== Stopwatch 示例 ===" << std::endl;

  // 1. 基本计时
  Stopwatch sw;
  DoWork();
  std::cout << "耗时: " << sw.ElapsedMilliseconds() << "ms" << std::endl;

  // 2. 重置
  sw.Reset();
  DoWork();
  std::cout << "重置后耗时: " << sw.ElapsedMilliseconds() << "ms" << std::endl;

  // 3. 暂停/恢复
  Stopwatch sw2;
  DoWork();
  sw2.Pause();
  std::cout << "暂停时耗时: " << sw2.ElapsedMilliseconds() << "ms" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "暂停期间耗时: " << sw2.ElapsedMilliseconds() << "ms" << std::endl;
  sw2.Resume();
  DoWork();
  std::cout << "恢复后耗时: " << sw2.ElapsedMilliseconds() << "ms" << std::endl;

  return 0;
}
