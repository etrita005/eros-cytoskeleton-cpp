#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "cytoskeleton/itc/message_queue/mq.h"

using namespace com::etrita::eros::cytos::itc::message_queue;
using namespace com::etrita::eros::cytos::object;

// Test message types
class TestMessage : public Message {
 public:
  TestMessage() = default;
  explicit TestMessage(int value) : value_(value) {}
  TestMessage(int what, int value) : Message(what), value_(value) {}

  int GetValue() const { return value_; }
  void SetValue(int value) { value_ = value; }

 private:
  int value_{0};
};

class AnotherTestMessage : public Message {
 public:
  explicit AnotherTestMessage(const std::string& data) : data_(data) {}

  const std::string& GetData() const { return data_; }

 private:
  std::string data_;
};

// Test Object subclass for wparam/lparam testing
class TestObject : public Object {
 public:
  explicit TestObject(int value) : value_(value) {}

  int GetValue() const { return value_; }

 private:
  int value_;
};

// Message Tests
TEST(MessageTest, DefaultConstructor) {
  auto msg = std::make_shared<Message>();
  EXPECT_EQ(msg->GetWhat(), 0);
  EXPECT_EQ(msg->GetWhen(), 0);
  EXPECT_EQ(msg->GetWParam(), nullptr);
  EXPECT_EQ(msg->GetLParam(), nullptr);
}

TEST(MessageTest, WhatConstructor) {
  auto msg = std::make_shared<Message>(42);
  EXPECT_EQ(msg->GetWhat(), 42);
}

TEST(MessageTest, FullConstructor) {
  auto wparam = std::make_shared<TestObject>(100);
  auto lparam = std::make_shared<TestObject>(200);
  auto msg = std::make_shared<Message>(1, wparam, lparam);

  EXPECT_EQ(msg->GetWhat(), 1);
  EXPECT_EQ(msg->GetWParam(), wparam);
  EXPECT_EQ(msg->GetLParam(), lparam);
}

TEST(MessageTest, SettersAndGetters) {
  auto msg = std::make_shared<Message>();

  msg->SetWhat(10);
  EXPECT_EQ(msg->GetWhat(), 10);

  auto wparam = std::make_shared<TestObject>(50);
  msg->SetWParam(wparam);
  EXPECT_EQ(msg->GetWParam(), wparam);

  auto lparam = std::make_shared<TestObject>(60);
  msg->SetLParam(lparam);
  EXPECT_EQ(msg->GetLParam(), lparam);

  msg->SetWhen(1000);
  EXPECT_EQ(msg->GetWhen(), 1000);
}

TEST(MessageTest, CustomMessageSubclass) {
  auto msg = std::make_shared<TestMessage>(42);
  EXPECT_EQ(msg->GetValue(), 42);

  msg->SetValue(100);
  EXPECT_EQ(msg->GetValue(), 100);
}

TEST(MessageTest, NotifyAndJoin) {
  auto msg = std::make_shared<Message>();

  std::thread t([msg]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    msg->Notify();
  });

  msg->Join();
  t.join();
}

TEST(MessageTest, JoinWithTimeout) {
  auto msg = std::make_shared<Message>();

  // Should timeout since we never notify
  bool result = msg->Join(std::chrono::milliseconds(50));
  EXPECT_FALSE(result);

  // Now notify and try again
  msg->Notify();
  result = msg->Join(std::chrono::milliseconds(50));
  EXPECT_TRUE(result);
}

// Handler Tests
TEST(HandlerTest, LambdaHandler) {
  std::atomic<bool> called{false};
  auto handler = std::make_shared<LambdaHandler<Message>>(
      [&called](std::shared_ptr<Message> msg) { called = true; });

  auto msg = std::make_shared<Message>();
  handler->Run(msg);

  EXPECT_TRUE(called);
}

TEST(HandlerTest, LambdaHandlerWithCustomMessage) {
  std::atomic<int> received_value{0};
  auto handler = std::make_shared<LambdaHandler<TestMessage>>(
      [&received_value](std::shared_ptr<TestMessage> msg) {
        received_value = msg->GetValue();
      });

  auto msg = std::make_shared<TestMessage>(42);
  handler->Run(msg);

  EXPECT_EQ(received_value, 42);
}

TEST(HandlerTest, ClassHandler) {
  class TestHandler : public Handler<TestMessage> {
   public:
    void Handle(std::shared_ptr<TestMessage> msg) override {
      received_value_ = msg->GetValue();
    }

    int GetReceivedValue() const { return received_value_; }

   private:
    int received_value_{0};
  };

  auto handler = std::make_shared<TestHandler>();
  auto msg = std::make_shared<TestMessage>(99);

  handler->Run(msg);
  EXPECT_EQ(handler->GetReceivedValue(), 99);
}

TEST(HandlerTest, AsyncHandler) {
  auto sync_handler = std::make_shared<LambdaHandler<Message>>(
      [](std::shared_ptr<Message> msg) {}, false);
  auto async_handler = std::make_shared<LambdaHandler<Message>>(
      [](std::shared_ptr<Message> msg) {}, true);

  EXPECT_FALSE(sync_handler->IsAsync());
  EXPECT_TRUE(async_handler->IsAsync());
}

TEST(HandlerTest, LightweightHandler) {
  std::atomic<int> wparam_value{0};
  std::atomic<int> lparam_value{0};

  auto handler = std::make_shared<LightweightHandler>(
      [&wparam_value, &lparam_value](Object::Ptr wparam, Object::Ptr lparam) {
        if (wparam) {
          wparam_value = std::static_pointer_cast<TestObject>(wparam)->GetValue();
        }
        if (lparam) {
          lparam_value = std::static_pointer_cast<TestObject>(lparam)->GetValue();
        }
      });

  auto wparam = std::make_shared<TestObject>(10);
  auto lparam = std::make_shared<TestObject>(20);
  auto msg = std::make_shared<Message>(1, wparam, lparam);

  handler->Run(msg);

  EXPECT_EQ(wparam_value, 10);
  EXPECT_EQ(lparam_value, 20);
}

// MessageQueue Tests
TEST(MessageQueueTest, EnqueueAndNext) {
  MessageQueue mq;

  auto msg = std::make_shared<Message>(1);
  EXPECT_TRUE(mq.EnqueueMessage(msg, 0));

  auto received = mq.Next();
  EXPECT_EQ(received->GetWhat(), 1);
}

TEST(MessageQueueTest, DelayedMessage) {
  MessageQueue mq;

  auto msg1 = std::make_shared<Message>(1);
  auto msg2 = std::make_shared<Message>(2);

  // Enqueue msg2 with 100ms delay, then msg1 immediately
  mq.EnqueueMessage(msg2, 100);
  mq.EnqueueMessage(msg1, 0);

  // First message should be msg1 (no delay)
  auto start = std::chrono::steady_clock::now();
  auto received1 = mq.Next();
  auto end = std::chrono::steady_clock::now();

  EXPECT_EQ(received1->GetWhat(), 1);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_LT(elapsed.count(), 50);  // Should be immediate

  // Second message should be msg2 (after ~100ms delay)
  start = std::chrono::steady_clock::now();
  auto received2 = mq.Next();
  end = std::chrono::steady_clock::now();

  EXPECT_EQ(received2->GetWhat(), 2);
  elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_GE(elapsed.count(), 50);  // Should have waited
}

TEST(MessageQueueTest, Quit) {
  MessageQueue mq;

  auto msg = std::make_shared<Message>(1);
  mq.EnqueueMessage(msg, 0);

  mq.Quit();

  auto received = mq.Next();
  EXPECT_EQ(received, nullptr);
}

TEST(MessageQueueTest, IsQuit) {
  MessageQueue mq;
  EXPECT_FALSE(mq.IsQuit());

  mq.Quit();
  EXPECT_TRUE(mq.IsQuit());
}

TEST(MessageQueueTest, Clean) {
  MessageQueue mq;

  mq.EnqueueMessage(std::make_shared<Message>(1), 0);
  mq.EnqueueMessage(std::make_shared<Message>(2), 0);

  mq.Clean();
  mq.Quit();

  auto received = mq.Next();
  EXPECT_EQ(received, nullptr);
}

// Looper Tests
TEST(LooperTest, CreateAndExit) {
  auto looper = std::make_shared<Looper>("TestLooper");
  EXPECT_FALSE(looper->IsRunning());

  looper->AsyncLoop();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(looper->IsRunning());

  looper->Exit();
  EXPECT_FALSE(looper->IsRunning());
}

TEST(LooperTest, PostAndReceive) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<bool> received{false};
  looper->RegisterHandler<Message>(
      [&received](std::shared_ptr<Message> msg) { received = true; });

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(received);
  looper->Exit();
}

TEST(LooperTest, PostWithArgs) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> received_value{0};
  looper->RegisterHandler<TestMessage>(
      [&received_value](std::shared_ptr<TestMessage> msg) {
        received_value = msg->GetValue();
      });

  looper->Post<TestMessage>(42);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(received_value, 42);
  looper->Exit();
}

TEST(LooperTest, SendSync) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> received_value{0};
  looper->RegisterHandler<TestMessage>(
      [&received_value](std::shared_ptr<TestMessage> msg) {
        received_value = msg->GetValue();
      });

  bool result = looper->Invoke<TestMessage>(99);

  EXPECT_TRUE(result);
  EXPECT_EQ(received_value, 99);
  looper->Exit();
}

TEST(LooperTest, PostDelayed) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<bool> received{false};
  looper->RegisterHandler<Message>(
      [&received](std::shared_ptr<Message> msg) { received = true; });

  auto start = std::chrono::steady_clock::now();
  looper->PostDelayed<Message>(std::chrono::milliseconds(200));

  // Wait for the delayed message
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  auto end = std::chrono::steady_clock::now();

  EXPECT_TRUE(received);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_GE(elapsed.count(), 150);  // Should have waited at least 150ms

  looper->Exit();
}

TEST(LooperTest, MultipleHandlers) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> count{0};

  looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });
  looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });
  looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(count, 3);
  looper->Exit();
}

TEST(LooperTest, UnregisterHandler) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> count{0};

  auto id1 = looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });
  [[maybe_unused]] auto id2 = looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(count, 2);

  looper->UnregisterHandler<Message>(id1);

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(count, 3);  // Only id2 should have been called

  looper->Exit();
}

TEST(LooperTest, LightweightMessage) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  constexpr int WM_TEST = 100;
  std::atomic<int> wparam_value{0};
  std::atomic<int> lparam_value{0};

  looper->RegisterHandler(
      WM_TEST,
      [&wparam_value, &lparam_value](Object::Ptr wparam, Object::Ptr lparam) {
        if (wparam) {
          wparam_value = std::static_pointer_cast<TestObject>(wparam)->GetValue();
        }
        if (lparam) {
          lparam_value = std::static_pointer_cast<TestObject>(lparam)->GetValue();
        }
      });

  auto wparam = std::make_shared<TestObject>(111);
  auto lparam = std::make_shared<TestObject>(222);
  looper->Post(WM_TEST, wparam, lparam);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(wparam_value, 111);
  EXPECT_EQ(lparam_value, 222);

  looper->Exit();
}

TEST(LooperTest, SendLightweightSync) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  constexpr int WM_TEST = 101;
  std::atomic<int> received_what{0};

  looper->RegisterHandler(WM_TEST,
                          [&received_what](Object::Ptr wparam, Object::Ptr lparam) {
                            received_what = 101;
                          });

  bool result = looper->Invoke(WM_TEST);

  EXPECT_TRUE(result);
  EXPECT_EQ(received_what, 101);

  looper->Exit();
}

TEST(LooperTest, AsyncHandler) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> thread_id{0};
  auto main_thread_id = std::this_thread::get_id();

  looper->RegisterHandler<Message>(
      [&thread_id, main_thread_id](std::shared_ptr<Message> msg) {
        // Check if we're running on a different thread
        if (std::this_thread::get_id() != main_thread_id) {
          thread_id = 1;
        }
      },
      true);  // async = true

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  EXPECT_EQ(thread_id, 1);
  looper->Exit();
}

TEST(LooperTest, MainLooper) {
  auto main_looper1 = Looper::GetMainLooper(false);
  auto main_looper2 = Looper::GetMainLooper(false);

  EXPECT_EQ(main_looper1, main_looper2);

  std::atomic<bool> received{false};
  main_looper1->RegisterHandler<Message>(
      [&received](std::shared_ptr<Message> msg) { received = true; });

  main_looper1->AsyncLoop();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  main_looper1->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(received);

  Looper::StopMainLooper();
}

TEST(LooperTest, InvokeWithTimeout) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> received_value{0};
  looper->RegisterHandler<TestMessage>(
      [&received_value](std::shared_ptr<TestMessage> msg) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        received_value = msg->GetValue();
      });

  // This should complete successfully
  bool result = looper->InvokeWithTimeout<TestMessage>(std::chrono::milliseconds(500), 42);

  EXPECT_TRUE(result);
  EXPECT_EQ(received_value, 42);

  looper->Exit();
}

TEST(LooperTest, DifferentMessageTypes) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> int_received{0};
  std::string string_received;

  looper->RegisterHandler<TestMessage>(
      [&int_received](std::shared_ptr<TestMessage> msg) {
        int_received = msg->GetValue();
      });

  looper->RegisterHandler<AnotherTestMessage>(
      [&string_received](std::shared_ptr<AnotherTestMessage> msg) {
        string_received = msg->GetData();
      });

  looper->Post<TestMessage>(123);
  looper->Post<AnotherTestMessage>(std::string("hello"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(int_received, 123);
  EXPECT_EQ(string_received, "hello");

  looper->Exit();
}

TEST(LooperTest, UnregisterAllHandlers) {
  auto looper = std::make_shared<Looper>("TestLooper", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> count{0};

  looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });
  looper->RegisterHandler<Message>(
      [&count](std::shared_ptr<Message> msg) { count++; });

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(count, 2);

  looper->UnregisterAllHandlers<Message>();

  looper->Post<Message>();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(count, 2);  // Should still be 2

  looper->Exit();
}

TEST(LooperTest, GetName) {
  auto looper = std::make_shared<Looper>("MyTestLooper");
  EXPECT_EQ(looper->GetName(), "MyTestLooper");
}

// Integration Tests
TEST(IntegrationTest, ComplexScenario) {
  auto looper = std::make_shared<Looper>("IntegrationTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> counter{0};

  // Register multiple handlers for different message types
  looper->RegisterHandler<TestMessage>(
      [&counter](std::shared_ptr<TestMessage> msg) { counter += msg->GetValue(); });

  looper->RegisterHandler<AnotherTestMessage>(
      [&counter](std::shared_ptr<AnotherTestMessage> msg) {
        counter += static_cast<int>(msg->GetData().length());
      });

  // Post various messages
  looper->Post<TestMessage>(10);
  looper->Post<AnotherTestMessage>(std::string("abc"));
  looper->Post<TestMessage>(20);
  looper->Invoke<TestMessage>(30);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // 10 + 3 + 20 + 30 = 63
  EXPECT_EQ(counter, 63);

  looper->Exit();
}

TEST(IntegrationTest, MultiThreadedPost) {
  auto looper = std::make_shared<Looper>("MultiThreadTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> counter{0};

  looper->RegisterHandler<Message>(
      [&counter](std::shared_ptr<Message> msg) { counter++; });

  // Post from multiple threads
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&looper]() {
      for (int j = 0; j < 10; ++j) {
        looper->Post<Message>();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_EQ(counter, 100);
  looper->Exit();
}

// PostHandler Tests
TEST(PostHandlerTest, BasicPostHandler) {
  auto looper = std::make_shared<Looper>("PostHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<bool> executed{false};
  looper->PostHandler([&executed]() { executed = true; });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(executed);

  looper->Exit();
}

TEST(PostHandlerTest, PostHandlerWithCapture) {
  auto looper = std::make_shared<Looper>("PostHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> value{0};
  int captured_value = 42;
  looper->PostHandler([&value, captured_value]() { value = captured_value; });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(value, 42);

  looper->Exit();
}

TEST(PostHandlerTest, PostHandlerDelayed) {
  auto looper = std::make_shared<Looper>("PostHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<bool> executed{false};
  auto start = std::chrono::steady_clock::now();

  looper->PostHandler([&executed]() { executed = true; },
                      std::chrono::milliseconds(200));

  // Should not execute immediately
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(executed);

  // Wait for execution
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  auto end = std::chrono::steady_clock::now();

  EXPECT_TRUE(executed);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_GE(elapsed.count(), 150);  // Should have waited at least 150ms

  looper->Exit();
}

TEST(PostHandlerTest, MultiplePostHandlers) {
  auto looper = std::make_shared<Looper>("PostHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> counter{0};

  looper->PostHandler([&counter]() { counter++; });
  looper->PostHandler([&counter]() { counter++; });
  looper->PostHandler([&counter]() { counter++; });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(counter, 3);

  looper->Exit();
}

TEST(PostHandlerTest, PostHandlerMixedWithRegularHandlers) {
  auto looper = std::make_shared<Looper>("PostHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> counter{0};

  // Register a regular handler
  looper->RegisterHandler<Message>(
      [&counter](std::shared_ptr<Message> msg) { counter += 10; });

  // Post a regular message
  looper->Post<Message>();

  // Post a lambda handler
  looper->PostHandler([&counter]() { counter += 1; });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(counter, 11);

  looper->Exit();
}

// InvokeHandler Tests
TEST(InvokeHandlerTest, BasicInvokeHandler) {
  auto looper = std::make_shared<Looper>("InvokeHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<bool> executed{false};
  bool result = looper->InvokeHandler([&executed]() { executed = true; });

  EXPECT_TRUE(result);
  EXPECT_TRUE(executed);

  looper->Exit();
}

TEST(InvokeHandlerTest, InvokeHandlerWithCapture) {
  auto looper = std::make_shared<Looper>("InvokeHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> value{0};
  int captured_value = 42;
  bool result = looper->InvokeHandler([&value, captured_value]() { value = captured_value; });

  EXPECT_TRUE(result);
  EXPECT_EQ(value, 42);

  looper->Exit();
}

TEST(InvokeHandlerTest, InvokeHandlerWithTimeout) {
  auto looper = std::make_shared<Looper>("InvokeHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> value{0};
  bool result = looper->InvokeHandler(
      [&value]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        value = 100;
      },
      std::chrono::milliseconds(200));

  EXPECT_TRUE(result);
  EXPECT_EQ(value, 100);

  looper->Exit();
}

TEST(InvokeHandlerTest, InvokeHandlerTimeoutExpired) {
  auto looper = std::make_shared<Looper>("InvokeHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<bool> executed{false};
  bool result = looper->InvokeHandler(
      [&executed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        executed = true;
      },
      std::chrono::milliseconds(50));

  EXPECT_FALSE(result);
  // Note: The handler may still execute after timeout, so we don't check 'executed'

  looper->Exit();
}

TEST(InvokeHandlerTest, MultipleInvokeHandlers) {
  auto looper = std::make_shared<Looper>("InvokeHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> counter{0};

  bool result1 = looper->InvokeHandler([&counter]() { counter++; });
  bool result2 = looper->InvokeHandler([&counter]() { counter++; });
  bool result3 = looper->InvokeHandler([&counter]() { counter++; });

  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);
  EXPECT_TRUE(result3);
  EXPECT_EQ(counter, 3);

  looper->Exit();
}

TEST(InvokeHandlerTest, InvokeHandlerMixedWithPostHandler) {
  auto looper = std::make_shared<Looper>("InvokeHandlerTest", true);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::atomic<int> counter{0};

  // Invoke synchronously
  bool result = looper->InvokeHandler([&counter]() { counter += 10; });
  EXPECT_TRUE(result);
  EXPECT_EQ(counter, 10);

  // Post asynchronously
  looper->PostHandler([&counter]() { counter += 1; });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(counter, 11);

  looper->Exit();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
