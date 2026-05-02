#include <iostream>

#include "cytoskeleton/time/time.h"

using namespace com::etrita::eros::cytos::time;

class TimerService {
 public:
  explicit TimerService(Clock* clock) : clock_(clock), start_(clock_->GetMonoTime()) {}

  bool IsTimeout(Duration timeout) {
    auto elapsed = clock_->GetMonoTime() - start_;
    return elapsed > timeout;
  }

  Duration Elapsed() const {
    return clock_->GetMonoTime() - start_;
  }

 private:
  Clock* clock_;
  MonoTime start_;
};

int main() {
  std::cout << "=== FakeClock 示例 ===" << std::endl;

  // 1. 创建 FakeClock
  FakeClock fake_clock;

  // 2. 注入到服务中
  TimerService service(&fake_clock);

  // 3. 初始状态
  std::cout << "初始耗时: " << service.Elapsed().ToMilliseconds() << "ms" << std::endl;
  std::cout << "是否超时 (5s): " << (service.IsTimeout(Duration::FromSeconds(5)) ? "是" : "否")
            << std::endl;

  // 4. 推进时间
  fake_clock.Advance(Duration::FromSeconds(3));
  std::cout << "推进 3s 后耗时: " << service.Elapsed().ToSeconds() << "s" << std::endl;
  std::cout << "是否超时 (5s): " << (service.IsTimeout(Duration::FromSeconds(5)) ? "是" : "否")
            << std::endl;

  // 5. 再推进 3s
  fake_clock.Advance(Duration::FromSeconds(3));
  std::cout << "再推进 3s 后耗时: " << service.Elapsed().ToSeconds() << "s" << std::endl;
  std::cout << "是否超时 (5s): " << (service.IsTimeout(Duration::FromSeconds(5)) ? "是" : "否")
            << std::endl;

  // 6. 独立推进 Wall 和 Mono
  fake_clock.AdvanceWall(Duration::FromHours(1));
  std::cout << "仅推进 Wall 后, WallTime: " << fake_clock.GetWallTime().Format() << std::endl;

  return 0;
}
