// Example: Thread-Safe Stack (LIFO)
// Demonstrates LIFO operations with blocking and non-blocking support

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/stack.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  // Example 1: Basic LIFO operations
  std::cout << "=== Basic LIFO Operations ===" << std::endl;
  {
    Stack<int> stack;

    stack.Push(10);
    stack.Push(20);
    stack.Push(30);

    std::cout << "Stack size: " << stack.Size() << std::endl;

    int val;
    while (stack.TryPop(val)) {
      std::cout << "Popped: " << val << std::endl;
    }
  }

  // Example 2: Expression evaluation (postfix)
  std::cout << "\n=== Postfix Expression Evaluation ===" << std::endl;
  {
    Stack<int> operands;
    std::string expression = "3 4 + 2 * 7 /";  // (3+4)*2/7 = 2

    auto apply_op = [&](char op) {
      int b, a;
      if (operands.TryPop(b) && operands.TryPop(a)) {
        switch (op) {
          case '+': operands.Push(a + b); break;
          case '-': operands.Push(a - b); break;
          case '*': operands.Push(a * b); break;
          case '/': operands.Push(a / b); break;
        }
      }
    };

    for (char c : expression) {
      if (c >= '0' && c <= '9') {
        operands.Push(c - '0');
      } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        apply_op(c);
      }
    }

    int result;
    if (operands.TryPop(result)) {
      std::cout << "Result: " << result << std::endl;
    }
  }

  // Example 3: Undo stack simulation
  std::cout << "\n=== Undo Stack Simulation ===" << std::endl;
  {
    Stack<std::string> undo_stack;
    std::string document = "";

    auto type = [&](const std::string& text) {
      undo_stack.Push(document);
      document += text;
      std::cout << "Typed: \"" << text << "\", Document: \"" << document << "\"" << std::endl;
    };

    auto undo = [&]() {
      std::string prev;
      if (undo_stack.TryPop(prev)) {
        document = prev;
        std::cout << "Undo! Document: \"" << document << "\"" << std::endl;
      } else {
        std::cout << "Nothing to undo" << std::endl;
      }
    };

    type("Hello");
    type(" World");
    type("!");
    undo();
    undo();
    type(" C++");
  }

  // Example 4: Blocking Pop (Producer-Consumer)
  std::cout << "\n=== Blocking Pop (Producer-Consumer) ===" << std::endl;
  {
    Stack<int> stack;
    const int num_items = 100;
    int sum = 0;

    std::thread producer([&]() {
      for (int i = 1; i <= num_items; ++i) {
        stack.Push(i);
      }
    });

    std::thread consumer([&]() {
      for (int i = 0; i < num_items; ++i) {
        int value;
        stack.Pop(value);  // Blocking - waits if stack is empty
        sum += value;
      }
    });

    producer.join();
    consumer.join();

    std::cout << "Sum of 1 to " << num_items << " = " << sum << std::endl;
    std::cout << "Expected: " << (num_items * (num_items + 1) / 2) << std::endl;
  }

  // Example 5: Filter operation
  std::cout << "\n=== Filter Operation ===" << std::endl;
  {
    Stack<int> stack;
    for (int i = 1; i <= 10; ++i) {
      stack.Push(i);
    }

    auto odds = stack.Filter([](const int& n) { return n % 2 == 1; });
    std::cout << "Odd numbers (LIFO order): ";
    for (int n : odds) {
      std::cout << n << " ";
    }
    std::cout << std::endl;
  }

  // Example 6: Concurrent pushes
  std::cout << "\n=== Concurrent Pushes ===" << std::endl;
  {
    Stack<int> stack;
    const int num_threads = 4;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&stack, i]() {
        for (int j = 0; j < 100; ++j) {
          stack.Push(i * 1000 + j);
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    std::cout << "Total elements: " << stack.Size() << std::endl;
  }

  return 0;
}
