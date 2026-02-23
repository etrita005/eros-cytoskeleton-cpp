// Example: Event Synchronization
// Demonstrates AutoResetEvent and ManualResetEvent for thread coordination

#include <chrono>
#include <iostream>
#include <thread>

#include "cytoskeleton/concurrent/event.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: ManualResetEvent - stays notified until manually reset
  std::cout << "=== ManualResetEvent Example ===" << std::endl;
  {
    ManualResetEvent event(false);

    std::thread worker([&]() {
      std::cout << "Worker: Waiting for notification..." << std::endl;
      event.Join();
      std::cout << "Worker: Received notification!" << std::endl;

      // Can wait again without re-notifying
      std::cout << "Worker: Waiting again (non-blocking)..." << std::endl;
      bool result = event.Join(std::chrono::milliseconds(100));
      std::cout << "Worker: Second wait result: " << (result ? "notified" : "timeout")
                << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Main: Notifying event" << std::endl;
    event.Notify();

    worker.join();
    event.Reset();  // Must manually reset
  }

  // Example 2: AutoResetEvent - auto resets after successful wait
  std::cout << "\n=== AutoResetEvent Example ===" << std::endl;
  {
    AutoResetEvent event(false);

    std::thread worker([&]() {
      std::cout << "Worker: Waiting for notification 1..." << std::endl;
      event.Join();
      std::cout << "Worker: Received notification 1!" << std::endl;

      std::cout << "Worker: Waiting for notification 2..." << std::endl;
      event.Join();
      std::cout << "Worker: Received notification 2!" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Main: Sending notification 1" << std::endl;
    event.Notify();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Main: Sending notification 2" << std::endl;
    event.Notify();

    worker.join();
  }

  // Example 3: Timeout handling
  std::cout << "\n=== Timeout Example ===" << std::endl;
  {
    ManualResetEvent event(false);

    std::cout << "Waiting with 200ms timeout..." << std::endl;
    bool result = event.Join(std::chrono::milliseconds(200));
    std::cout << "Result: " << (result ? "notified" : "timeout") << std::endl;

    event.Notify();
    std::cout << "Waiting with 200ms timeout (after notify)..." << std::endl;
    result = event.Join(std::chrono::milliseconds(200));
    std::cout << "Result: " << (result ? "notified" : "timeout") << std::endl;
  }

  return 0;
}
