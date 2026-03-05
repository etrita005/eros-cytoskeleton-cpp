# Concurrent 模块使用文档

## 概述

Concurrent 模块是一个 header-only 的并发库，专为机器人应用和服务开发设计。基于 C++20 标准，提供线程安全的容器、同步原语和线程池等功能。

## 命名空间

```cpp
namespace com::etrita::eros::cytos::concurrent;
```

## 核心特性

1. **Header-Only**: 所有实现都在头文件中，无需编译链接
2. **C++20**: 使用 `std::jthread`、`std::stop_token` 等现代特性
3. **线程安全**: 所有容器默认线程安全
4. **RAII 模式**: 自动资源管理
5. **最小开销**: 标准库原语的轻量级封装

## 快速开始

### 引入依赖

```cpp
#include "cytoskeleton/concurrent/concurrent.h"

using namespace com::etrita::eros::cytos::concurrent;
```

### Bazel 配置

```python
# BUILD.bazel
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = ["@cytoskeleton//include/cytoskeleton/concurrent:concurrent"],
)
```

## 功能模块

### 1. 互斥锁与同步

#### Mutex - 递归互斥锁

**基本用法**

```cpp
#include "cytoskeleton/concurrent/mutex.h"

Mutex mutex;

// 手动加锁/解锁
mutex.Lock();
// 临界区
mutex.Unlock();

// 推荐：使用 RAII 锁守卫
{
  MutexLock lock(mutex);
  // 临界区 - 离开作用域自动解锁
}
```

**线程安全计数器示例**

```cpp
class SafeCounter {
 public:
  void Increment() {
    MutexLock lock(mutex_);
    ++count_;
  }

  int GetCount() const {
    MutexLock lock(mutex_);
    return count_;
  }

 private:
  mutable Mutex mutex_;
  int count_ = 0;
};

// 多线程使用
auto counter = std::make_shared<SafeCounter>();
std::vector<std::thread> threads;

for (int i = 0; i < 4; ++i) {
  threads.emplace_back([counter]() {
    for (int j = 0; j < 1000; ++j) {
      counter->Increment();
    }
  });
}

for (auto& t : threads) {
  t.join();
}

std::cout << "Final count: " << counter->GetCount() << std::endl;  // 4000
```

#### ReadWriteMutex - 读写锁

适用于读多写少的场景，允许多个读线程同时访问，写线程独占访问。

```cpp
#include "cytoskeleton/concurrent/mutex.h"

ReadWriteMutex rw_mutex;
int data = 0;

// 读操作 - 可多个线程同时读
{
  ReadLock lock(rw_mutex);
  std::cout << "Reading: " << data << std::endl;
}

// 写操作 - 独占访问
{
  WriteLock lock(rw_mutex);
  data = 42;
}
```

**线程安全缓存示例**

```cpp
class ThreadSafeCache {
 public:
  int Get(const std::string& key) {
    ReadLock lock(mutex_);
    auto it = cache_.find(key);
    return (it != cache_.end()) ? it->second : -1;
  }

  void Set(const std::string& key, int value) {
    WriteLock lock(mutex_);
    cache_[key] = value;
  }

 private:
  mutable ReadWriteMutex mutex_;
  std::unordered_map<std::string, int> cache_;
};
```

### 2. 事件同步

#### ManualResetEvent - 手动重置事件

事件保持通知状态直到手动调用 `Reset()`。

```cpp
#include "cytoskeleton/concurrent/event.h"

ManualResetEvent event(false);  // 初始未通知

std::thread worker([&event]() {
  std::cout << "Waiting..." << std::endl;
  event.Join();  // 等待通知
  std::cout << "Notified!" << std::endl;
  
  // 仍可再次等待（事件保持通知状态）
  event.Join();  // 不会阻塞
});

std::this_thread::sleep_for(std::chrono::seconds(1));
event.Notify();  // 通知所有等待者

worker.join();
event.Reset();   // 手动重置为未通知状态
```

#### AutoResetEvent - 自动重置事件

每次通知只唤醒一个等待线程，然后自动重置。

```cpp
AutoResetEvent event(false);

std::thread worker1([&event]() {
  event.Join();
  std::cout << "Worker 1 notified" << std::endl;
});

std::thread worker2([&event]() {
  event.Join();
  std::cout << "Worker 2 notified" << std::endl;
});

std::this_thread::sleep_for(std::chrono::milliseconds(100));
event.Notify();  // 只唤醒一个线程
event.Notify();  // 再唤醒另一个线程

worker1.join();
worker2.join();
```

**带超时的等待**

```cpp
ManualResetEvent event(false);

// 等待 200ms，超时返回 false
bool result = event.Join(std::chrono::milliseconds(200));
if (!result) {
  std::cout << "Timeout!" << std::endl;
}
```

### 3. 线程管理

#### Thread - jthread 封装

```cpp
#include "cytoskeleton/concurrent/thread.h"

Thread thread([](std::stop_token st) {
  while (!st.stop_requested()) {
    std::cout << "Working..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
});

// 自动停止并加入
// 析构时会自动调用 request_stop() 和 join()
```

#### ThreadPool - 线程池

基于 `boost::asio::thread_pool` 实现的任务池。

```cpp
#include "cytoskeleton/concurrent/thread_pool.h"

ThreadPool pool(4);  // 4 个线程

// 提交带返回值的任务
auto future = pool.Submit([]() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return 42;
});

std::cout << "Result: " << future.get() << std::endl;

// 提交带参数的任务
auto sum_future = pool.Submit([](int n) {
  int sum = 0;
  for (int i = 1; i <= n; ++i) {
    sum += i;
  }
  return sum;
}, 100);

std::cout << "Sum 1-100: " << sum_future.get() << std::endl;

// 提交无返回值任务
pool.Submit([]() {
  std::cout << "Void task" << std::endl;
}).get();  // 等待完成
```

**并行计算示例**

```cpp
ThreadPool pool(4);
std::vector<std::future<long long>> futures;

// 提交 8 个并行任务
for (int i = 0; i < 8; ++i) {
  futures.push_back(pool.Submit([i]() -> long long {
    long long sum = 0;
    for (long long j = 0; j < 1000000; ++j) {
      sum += j;
    }
    return sum;
  }));
}

// 收集结果
for (size_t i = 0; i < futures.size(); ++i) {
  std::cout << "Task " << i << " result: " << futures[i].get() << std::endl;
}
```

### 4. 线程安全容器

所有容器都是线程安全的，内部使用互斥锁保护。

#### Vector - 动态数组

```cpp
#include "cytoskeleton/concurrent/vector.h"

Vector<int> vec;

// 添加元素
vec.PushBack(1);
vec.PushBack(2);
vec.PushBack(3);

// 访问元素
std::cout << "Size: " << vec.Size() << std::endl;
std::cout << "First: " << vec[0] << std::endl;

// 遍历
for (size_t i = 0; i < vec.Size(); ++i) {
  std::cout << vec[i] << " ";
}

// 范围 for 循环（需要拷贝）
for (int val : vec.ToVector()) {
  std::cout << val << " ";
}
```

#### Map - 有序映射

```cpp
#include "cytoskeleton/concurrent/map.h"

Map<std::string, int> map;

// 插入元素
map.Insert("one", 1);
map.Insert("two", 2);

// 访问元素
if (auto val = map.Find("one")) {
  std::cout << "one = " << *val << std::endl;
}

// 更新元素
map.Insert("one", 100);

// 遍历
auto items = map.ToMap();
for (const auto& [key, val] : items) {
  std::cout << key << " = " << val << std::endl;
}
```

#### HashMap - 哈希映射

```cpp
#include "cytoskeleton/concurrent/hash_map.h"

HashMap<int, std::string> hash_map;

hash_map.Insert(1, "one");
hash_map.Insert(2, "two");

if (auto val = hash_map.Find(1)) {
  std::cout << "1 = " << *val << std::endl;
}
```

#### Queue - 队列

```cpp
#include "cytoskeleton/concurrent/queue.h"

Queue<int> queue;

queue.Push(1);
queue.Push(2);
queue.Push(3);

std::cout << "Front: " << queue.Front() << std::endl;  // 1
std::cout << "Back: " << queue.Back() << std::endl;    // 3

int val = queue.Pop();  // 移除并返回队首元素
```

#### Stack - 栈

```cpp
#include "cytoskeleton/concurrent/stack.h"

Stack<int> stack;

stack.Push(1);
stack.Push(2);
stack.Push(3);

std::cout << "Top: " << stack.Top() << std::endl;  // 3

int val = stack.Pop();  // 移除并返回栈顶元素
```

#### List - 双向链表

```cpp
#include "cytoskeleton/concurrent/list.h"

List<int> list;

list.PushBack(1);
list.PushBack(2);
list.PushFront(0);

for (int val : list.ToList()) {
  std::cout << val << " ";  // 0 1 2
}
```

#### Tree - 树

```cpp
#include "cytoskeleton/concurrent/tree.h"

Tree tree;

// 添加节点
auto root = tree.AddRoot(1);
auto child1 = tree.AddChild(root, 2);
auto child2 = tree.AddChild(root, 3);

// 遍历
for (const auto& node : tree.BfsTraversal()) {
  std::cout << node->GetValue() << " ";  // 1 2 3
}
```

## 完整示例

### 生产者 - 消费者模式

```cpp
#include <iostream>
#include <chrono>

#include "cytoskeleton/concurrent/concurrent.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  Queue<int> queue;
  ManualResetEvent stop_event(false);
  const int MAX_SIZE = 10;

  // 生产者线程
  Thread producer([&](std::stop_token st) {
    int count = 0;
    while (!st.stop_requested() && !stop_event.Join(std::chrono::milliseconds(0))) {
      {
        MutexLock lock(queue.GetMutex());
        if (queue.Size() < MAX_SIZE) {
          queue.Push(++count);
          std::cout << "Produced: " << count << std::endl;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  // 消费者线程
  Thread consumer([&](std::stop_token st) {
    while (!st.stop_requested()) {
      int value;
      {
        MutexLock lock(queue.GetMutex());
        if (queue.Size() > 0) {
          value = queue.Pop();
          std::cout << "Consumed: " << value << std::endl;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
  });

  // 运行 5 秒后停止
  std::this_thread::sleep_for(std::chrono::seconds(5));
  stop_event.Notify();
  
  // 等待线程结束
  producer.Join();
  consumer.Join();

  return 0;
}
```

### 使用线程池处理并发任务

```cpp
#include <iostream>
#include <vector>

#include "cytoskeleton/concurrent/thread_pool.h"
#include "cytoskeleton/concurrent/vector.h"

using namespace com::etrita::eros::cytos::concurrent;

int main() {
  ThreadPool pool(4);
  Vector<int> results;

  // 提交 20 个任务
  std::vector<std::future<void>> futures;
  for (int i = 0; i < 20; ++i) {
    futures.push_back(pool.Submit([i, &results]() {
      int result = i * i;
      results.PushBack(result);
      std::cout << "Task " << i << " completed, result: " << result << std::endl;
    }));
  }

  // 等待所有任务完成
  for (auto& f : futures) {
    f.get();
  }

  std::cout << "All tasks completed. Total results: " << results.Size() << std::endl;

  return 0;
}
```

## 最佳实践

### 1. 优先使用 RAII 锁守卫

```cpp
// 推荐
void SafeFunction() {
  MutexLock lock(mutex);
  // 临界区
}

// 不推荐
void UnsafeFunction() {
  mutex.Lock();
  // 临界区
  mutex.Unlock();  // 如果中间抛出异常，锁永远不会释放
}
```

### 2. 选择合适的容器

- **Vector**: 随机访问频繁，尾部插入/删除
- **List**: 频繁插入/删除，不需要随机访问
- **Map**: 需要有序键值对
- **HashMap**: 需要快速查找，不关心顺序
- **Queue**: FIFO 场景
- **Stack**: LIFO 场景

### 3. 读写锁优化读性能

对于读多写少的场景，使用 `ReadWriteMutex` 代替 `Mutex`：

```cpp
// 读多写少的缓存
class Cache {
 public:
  int Get(const std::string& key) {
    ReadLock lock(mutex_);  // 多个读线程可并发
    return cache_[key];
  }

  void Set(const std::string& key, int value) {
    WriteLock lock(mutex_);  // 写时独占
    cache_[key] = value;
  }

 private:
  mutable ReadWriteMutex mutex_;
  std::unordered_map<std::string, int> cache_;
};
```

### 4. 使用线程池代替手动创建线程

```cpp
// 推荐：使用线程池
ThreadPool pool(4);
for (int i = 0; i < 100; ++i) {
  pool.Submit([i]() { DoWork(i); });
}

// 不推荐：手动创建大量线程
std::vector<std::thread> threads;
for (int i = 0; i < 100; ++i) {
  threads.emplace_back([i]() { DoWork(i); });
}
```

### 5. 正确处理事件通知

```cpp
// 推荐：使用超时避免永久阻塞
if (!event.Join(std::chrono::seconds(5))) {
  std::cerr << "Timeout waiting for event" << std::endl;
}

// 推荐：检查停止标记
Thread worker([&](std::stop_token st) {
  while (!st.stop_requested()) {
    // 工作
  }
});
```

## 相关文档

- [架构设计文档](../architecture/concurrent_requirements.md)
- [示例代码](../../examples/concurrent/)
- [Object 模块使用文档](object_usage.md)
