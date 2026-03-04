// Basic Message Queue Example
// Demonstrates the fundamental usage of the message queue system

#include <iostream>
#include <string>

#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;
using namespace com::etrita::eros::cytos::object;

// Define custom message types
class ClickMessage : public Message {
 public:
  ClickMessage(int x, int y) : x_(x), y_(y) {}

  int GetX() const { return x_; }
  int GetY() const { return y_; }

 private:
  int x_;
  int y_;
};

class DataMessage : public Message {
 public:
  explicit DataMessage(const std::string& data) : data_(data) {}

  const std::string& GetData() const { return data_; }

 private:
  std::string data_;
};

// Object for wparam/lparam
class Point : public Object {
 public:
  Point(int x, int y) : x_(x), y_(y) {}

  int GetX() const { return x_; }
  int GetY() const { return y_; }

 private:
  int x_;
  int y_;
};

// Message type constants
constexpr int MSG_TYPE_CLICK = 1;
constexpr int MSG_TYPE_DATA = 2;
constexpr int WM_CLICK = 100;
constexpr int WM_DATA = 101;

// Forward declarations for message types
class ClickMessage;
class DataMessage;

int main() {
  std::cout << "=== Message Queue Basic Example ===" << std::endl;

  // Create and start a looper
  auto looper = std::make_shared<Looper>("ExampleLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // ============================================
  // Example 1: Lambda Handler with Custom Message
  // ============================================
  std::cout << "\n[Example 1] Lambda Handler with Custom Message" << std::endl;

  looper->RegisterHandler<ClickMessage>([](std::shared_ptr<ClickMessage> msg) {
    std::cout << "Click at (" << msg->GetX() << ", " << msg->GetY() << ")" << std::endl;
  });

  // Post a click message
  looper->Post<ClickMessage>(100, 200);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // ============================================
  // Example 2: Multiple Handlers for Same Message Type
  // ============================================
  std::cout << "\n[Example 2] Multiple Handlers" << std::endl;

  looper->RegisterHandler<DataMessage>([](std::shared_ptr<DataMessage> msg) {
    std::cout << "Handler 1 received: " << msg->GetData() << std::endl;
  });

  looper->RegisterHandler<DataMessage>([](std::shared_ptr<DataMessage> msg) {
    std::cout << "Handler 2 received: " << msg->GetData() << std::endl;
  });

  looper->Post<DataMessage>(std::string("Hello, World!"));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // ============================================
  // Example 3: Synchronous Invoke
  // ============================================
  std::cout << "\n[Example 3] Synchronous Invoke" << std::endl;

  std::atomic<int> result{0};
  looper->RegisterHandler<ClickMessage>([&result](std::shared_ptr<ClickMessage> msg) {
    result = msg->GetX() + msg->GetY();
    std::cout << "Processing click synchronously..." << std::endl;
  });

  bool success = looper->Invoke<ClickMessage>(50, 50);
  std::cout << "Invoke result: " << (success ? "success" : "failed") << std::endl;
  std::cout << "Result: " << result << std::endl;

  // ============================================
  // Example 4: Delayed Message
  // ============================================
  std::cout << "\n[Example 4] Delayed Message" << std::endl;

  std::atomic<bool> delayed_received{false};
  looper->RegisterHandler<Message>(
      [&delayed_received](std::shared_ptr<Message> msg) {
        delayed_received = true;
        std::cout << "Delayed message received!" << std::endl;
      });

  auto start = std::chrono::steady_clock::now();
  looper->PostDelayed<Message>(std::chrono::milliseconds(500));

  std::cout << "Waiting for delayed message..." << std::endl;
  while (!delayed_received) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  auto end = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Delayed message received after " << elapsed.count() << "ms" << std::endl;

  // ============================================
  // Example 5: Lightweight Messages (Windows-style)
  // ============================================
  std::cout << "\n[Example 5] Lightweight Messages (Windows-style)" << std::endl;

  looper->RegisterHandler(WM_CLICK, [](Object::Ptr wparam, Object::Ptr lparam) {
    if (wparam) {
      auto point = std::static_pointer_cast<Point>(wparam);
      std::cout << "WM_CLICK: Point(" << point->GetX() << ", " << point->GetY() << ")"
                << std::endl;
    }
  });

  looper->RegisterHandler(WM_DATA, [](Object::Ptr wparam, Object::Ptr lparam) {
    if (wparam) {
      auto point = std::static_pointer_cast<Point>(wparam);
      std::cout << "WM_DATA: Point(" << point->GetX() << ", " << point->GetY() << ")"
                << std::endl;
    }
  });

  auto point1 = std::make_shared<Point>(300, 400);
  looper->Post(WM_CLICK, point1, nullptr);

  auto point2 = std::make_shared<Point>(500, 600);
  looper->Post(WM_DATA, point2, nullptr);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // ============================================
  // Example 6: Async Handler
  // ============================================
  std::cout << "\n[Example 6] Async Handler" << std::endl;

  std::atomic<int> async_thread_id{0};
  auto main_thread_id = std::this_thread::get_id();

  looper->RegisterHandler<Message>(
      [&async_thread_id, main_thread_id](std::shared_ptr<Message> msg) {
        if (std::this_thread::get_id() != main_thread_id) {
          async_thread_id = 1;
          std::cout << "Async handler executed on different thread" << std::endl;
        }
      },
      true);  // async = true

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // ============================================
  // Example 7: Handler Registration and Unregistration
  // ============================================
  std::cout << "\n[Example 7] Handler Registration/ Unregistration" << std::endl;

  std::atomic<int> counter{0};

  auto handler_id = looper->RegisterHandler<Message>(
      [&counter](std::shared_ptr<Message> msg) { counter++; });

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "Counter after first post: " << counter << std::endl;

  looper->UnregisterHandler<Message>(handler_id);
  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "Counter after unregister (should be same): " << counter << std::endl;

  // ============================================
  // Example 8: PostHandler (Direct Lambda Execution)
  // ============================================
  std::cout << "\n[Example 8] PostHandler (Direct Lambda Execution)" << std::endl;

  // Direct lambda execution without registering a handler
  looper->PostHandler([]() {
    std::cout << "PostHandler: Hello World!" << std::endl;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Delayed lambda execution
  std::atomic<bool> delayed_handler_executed{false};
  looper->PostHandler([&delayed_handler_executed]() {
    std::cout << "PostHandler: Delayed execution!" << std::endl;
    delayed_handler_executed = true;
  }, std::chrono::milliseconds(300));

  std::cout << "Waiting for delayed PostHandler..." << std::endl;
  auto ph_start = std::chrono::steady_clock::now();
  while (!delayed_handler_executed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  auto ph_end = std::chrono::steady_clock::now();
  auto ph_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(ph_end - ph_start);
  std::cout << "Delayed PostHandler executed after " << ph_elapsed.count() << "ms" << std::endl;

  // ============================================
  // Example 9: InvokeHandler (Synchronous Lambda Execution)
  // ============================================
  std::cout << "\n[Example 9] InvokeHandler (Synchronous Lambda Execution)" << std::endl;

  // Synchronous lambda execution - blocks until execution completes
  std::atomic<int> sync_value{0};
  bool invoke_result = looper->InvokeHandler([&sync_value]() {
    std::cout << "InvokeHandler: Synchronous execution!" << std::endl;
    sync_value = 123;
  });
  std::cout << "InvokeHandler result: " << (invoke_result ? "success" : "failed") << std::endl;
  std::cout << "Sync value after InvokeHandler: " << sync_value << std::endl;

  // Synchronous lambda with timeout
  bool invoke_timeout_result = looper->InvokeHandler([]() {
    std::cout << "InvokeHandler: Synchronous with timeout!" << std::endl;
  }, std::chrono::milliseconds(500));
  std::cout << "InvokeHandler with timeout result: "
            << (invoke_timeout_result ? "success" : "failed") << std::endl;

  // ============================================
  // Cleanup
  // ============================================
  std::cout << "\n=== Cleaning up ===" << std::endl;
  looper->Exit();
  std::cout << "Looper stopped. Example completed." << std::endl;

  return 0;
}
