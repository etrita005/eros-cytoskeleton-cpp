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

**使用 Lambda 函数创建线程**

```cpp
#include "cytoskeleton/concurrent/thread.h"

Thread thread("worker_thread", [](std::stop_token st) {
  while (!st.stop_requested()) {
    std::cout << "Working..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
});

thread.Start();

// 等待一段时间后请求停止
std::this_thread::sleep_for(std::chrono::seconds(1));
thread.RequestStop();
thread.Join();
```

**自动停止和加入**

```cpp
{
  Thread thread("auto_thread", [](std::stop_token st) {
    while (!st.stop_requested()) {
      // 工作
    }
  });
  thread.Start();
  // 离开作用域时自动调用 RequestStop() 和 Join()
}
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

#### Queue - 队列（支持阻塞等待）

```cpp
#include "cytoskeleton/concurrent/queue.h"

Queue<int> queue;

// 生产者：入队
queue.Enqueue(1);
queue.Enqueue(2);
queue.Enqueue(3);

// 消费者：阻塞出队（队列为空时等待）
int value;
queue.Dequeue(value);  // 阻塞直到有数据
std::cout << "Dequeued: " << value << std::endl;

// 非阻塞出队
if (queue.TryDequeue(value)) {
  std::cout << "TryDequeue: " << value << std::endl;
}
```

**生产者-消费者示例**

```cpp
Queue<int> queue;

// 生产者线程
Thread producer("producer", [&queue](std::stop_token st) {
  int count = 0;
  while (!st.stop_requested()) {
    queue.Enqueue(++count);
    std::cout << "Produced: " << count << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
});

// 消费者线程
Thread consumer("consumer", [&queue](std::stop_token st) {
  while (!st.stop_requested()) {
    int value;
    if (queue.Dequeue(value)) {
      std::cout << "Consumed: " << value << std::endl;
    }
  }
});

producer.Start();
consumer.Start();

std::this_thread::sleep_for(std::chrono::seconds(5));
producer.RequestStop();
consumer.RequestStop();
producer.Join();
consumer.Join();
```

#### Stack - 栈（支持阻塞等待）

```cpp
#include "cytoskeleton/concurrent/stack.h"

Stack<int> stack;

// 入栈
stack.Push(1);
stack.Push(2);
stack.Push(3);

// 阻塞出栈（栈为空时等待）
int value;
stack.Pop(value);  // 阻塞直到有数据
std::cout << "Popped: " << value << std::endl;

// 非阻塞出栈
if (stack.TryPop(value)) {
  std::cout << "TryPop: " << value << std::endl;
}

// 查看栈顶元素但不移除
if (stack.TryGet(value)) {
  std::cout << "Top: " << value << std::endl;
}
```

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
  std::atomic<bool> stop{false};

  // 生产者线程
  Thread producer("producer", [&queue, &stop](std::stop_token st) {
    int count = 0;
    while (!st.stop_requested() && !stop.load()) {
      queue.Enqueue(++count);
      std::cout << "Produced: " << count << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  // 消费者线程
  Thread consumer("consumer", [&queue, &stop](std::stop_token st) {
    while (!st.stop_requested() && !stop.load()) {
      int value;
      if (queue.Dequeue(value)) {
        std::cout << "Consumed: " << value << std::endl;
      }
    }
  });

  producer.Start();
  consumer.Start();

  // 运行 5 秒后停止
  std::this_thread::sleep_for(std::chrono::seconds(5));
  stop.store(true);
  
  // 等待线程结束
  producer.RequestStop();
  consumer.RequestStop();
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
- **Queue**: FIFO 场景，支持阻塞等待
- **Stack**: LIFO 场景，支持阻塞等待

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
Thread worker("worker", [&](std::stop_token st) {
  while (!st.stop_requested()) {
    // 工作
  }
});
```

### 6. Queue 和 Stack 的阻塞 vs 非阻塞

```cpp
Queue<int> queue;

// 阻塞操作 - 适合消费者线程
int value;
queue.Dequeue(value);  // 队列为空时阻塞等待

// 非阻塞操作 - 适合轮询场景
if (queue.TryDequeue(value)) {
  // 成功获取数据
} else {
  // 队列为空，可以做其他事情
}
```

## 相关文档

- [架构设计文档](../architecture/concurrent_requirements.md)
- [示例代码](../../examples/concurrent/)
- [Object 模块使用文档](object_usage.md)
