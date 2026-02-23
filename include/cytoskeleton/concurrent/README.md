# Cytoskeleton Concurrent Module

Header-only concurrent library for robot applications and service development.

## Design Philosophy

### Core Principles

1. **Header-Only Design**: All implementations in headers for easy integration
2. **Modern C++20**: Leverages `std::jthread`, `std::stop_token`, and other C++20 features
3. **Thread Safety First**: All containers are thread-safe by design
4. **RAII Pattern**: Resource management through constructors/destructors
5. **Minimal Overhead**: Thin wrappers around standard library primitives

### Architecture

```
com::etrita::eros::cytos::concurrent
├── Synchronization Primitives
│   ├── Mutex (recursive)
│   ├── ReadWriteMutex
│   ├── MutexLock, ReadLock, WriteLock (RAII guards)
│   ├── AutoResetEvent
│   └── ManualResetEvent
├── Threading
│   ├── Thread (jthread wrapper)
│   └── ThreadPool (boost::asio::thread_pool based)
└── Containers
    ├── Vector<T>
    ├── List<T>
    ├── Queue<T>
    ├── Stack<T>
    ├── Map<K, V>
    ├── HashMap<K, V>
    └── Tree
```

## Usage

### Bazel Integration

```python
# MODULE.bazel
bazel_dep(name = "cytoskeleton", version = "0.1.0")

# BUILD.bazel
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = ["@cytoskeleton//include/cytoskeleton/concurrent:concurrent"],
)
```

### Include

```cpp
#include "cytoskeleton/concurrent/concurrent.h"

using namespace com::etrita::eros::cytos::concurrent;
```

## API Reference

### 1. Mutex & Synchronization

#### Mutex

Recursive mutex wrapper with RAII lock guards.

```cpp
class Mutex {
 public:
  using Ptr = std::shared_ptr<Mutex>;
  
  Mutex();
  ~Mutex();
  
  void Lock();      // Acquire lock (blocks)
  void Unlock();    // Release lock
  bool TryLock();   // Try acquire (non-blocking)
};
```

**Usage:**
```cpp
Mutex::Ptr mutex = std::make_shared<Mutex>();
mutex->Lock();
// Critical section
mutex->Unlock();

// Or use RAII guard
MutexLock lock(*mutex);  // Auto-unlock on scope exit
```

#### ReadWriteMutex

Shared/exclusive lock for read-heavy workloads.

```cpp
class ReadWriteMutex {
 public:
  using Ptr = std::shared_ptr<ReadWriteMutex>;
  
  void LockRead();     // Shared lock
  void UnlockRead();
  bool TryLockRead();
  
  void LockWrite();    // Exclusive lock
  void UnlockWrite();
  bool TryLockWrite();
};
```

**Usage:**
```cpp
ReadWriteMutex::Ptr rw_mutex = std::make_shared<ReadWriteMutex>();

// Read operation
{
  ReadLock lock(*rw_mutex);  // Multiple readers allowed
  // Read data
}

// Write operation
{
  WriteLock lock(*rw_mutex);  // Exclusive access
  // Modify data
}
```

### 2. Events

#### AutoResetEvent

Auto-reset event for one-to-one synchronization.

```cpp
class AutoResetEvent : public Event {
 public:
  using Ptr = std::shared_ptr<AutoResetEvent>;
  
  explicit AutoResetEvent(bool initial_state = false);
  
  void Notify();                    // Signal the event
  void Reset();                     // Reset to non-signaled
  void Join();                      // Wait indefinitely
  bool Join(std::chrono::milliseconds timeout);  // Wait with timeout
  bool IsNotified() const;          // Check state
};
```

**Usage:**
```cpp
AutoResetEvent::Ptr event = std::make_shared<AutoResetEvent>(false);

// Thread 1: Wait for signal
event->Join();

// Thread 2: Signal
event->Notify();  // Auto-reset after successful wait
```

#### ManualResetEvent

Manual-reset event for one-to-many synchronization.

```cpp
class ManualResetEvent : public Event {
 public:
  using Ptr = std::shared_ptr<ManualResetEvent>;
  
  explicit ManualResetEvent(bool initial_state = false);
  
  void Notify();
  void Reset();
  void Join();
  bool Join(std::chrono::milliseconds timeout);
  bool IsNotified() const;
};
```

**Usage:**
```cpp
ManualResetEvent::Ptr event = std::make_shared<ManualResetEvent>(false);

event->Notify();   // Signal all waiters
// Multiple threads can wait and all will be released
event->Reset();    // Must manually reset
```

### 3. Threading

#### Thread

`std::jthread` wrapper with stop token support.

```cpp
class Thread {
 public:
  using Ptr = std::shared_ptr<Thread>;
  
  explicit Thread(const std::string& name);
  Thread(const std::string& name, std::function<void(std::stop_token)> func);
  
  void Start();           // Start the thread
  void Join();            // Wait for completion
  bool Join(std::chrono::milliseconds timeout);
  void RequestStop();     // Request graceful stop
  bool ShouldStop() const;
  std::string GetName() const;
  
  virtual void Run(std::stop_token stop_token);  // Override for custom logic
};
```

**Usage:**
```cpp
// Method 1: Inheritance
class Worker : public Thread {
 public:
  Worker() : Thread("Worker") {}
  
  void Run(std::stop_token stop_token) override {
    while (!stop_token.stop_requested()) {
      // Do work
    }
  }
};

Thread::Ptr worker = std::make_shared<Worker>();
worker->Start();
worker->RequestStop();
worker->Join();

// Method 2: Lambda
Thread::Ptr thread = std::make_shared<Thread>("LambdaWorker",
  [](std::stop_token token) {
    while (!token.stop_requested()) {
      // Do work
    }
  });
thread->Start();
```

#### ThreadPool

Fixed-size thread pool based on `boost::asio::thread_pool`.

```cpp
class ThreadPool {
 public:
  using Ptr = std::shared_ptr<ThreadPool>;
  
  explicit ThreadPool(size_t pool_size);
  ~ThreadPool();
  
  template <typename F, typename... Args>
  auto Submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
  
  void Shutdown();      // Stop accepting new tasks, wait for completion
  void Join();          // Wait for all tasks to complete
  bool IsRunning() const;
};
```

**Usage:**
```cpp
ThreadPool::Ptr pool = std::make_shared<ThreadPool>(4);  // 4 threads

// Submit task with return value
auto future = pool->Submit([](int x) { return x * x; }, 5);
int result = future.get();  // 25

// Submit void task
auto future2 = pool->Submit([]() {
  std::cout << "Hello from thread pool" << std::endl;
});
future2.wait();

pool->Shutdown();
```

### 4. Containers

All containers provide:
- Thread-safe operations
- `Ptr` type alias for `std::shared_ptr`
- `ForEach()` iteration
- `Filter()` and `Slice()` operations (where applicable)
- Move semantics support

#### Vector<T>

Thread-safe dynamic array.

```cpp
template <typename T>
class Vector {
 public:
  using Ptr = std::shared_ptr<Vector<T>>;
  
  void PushBack(const T& value);
  void PushBack(T&& value);
  void PushFront(const T& value);
  void PushFront(T&& value);
  bool PopBack(T& out);
  bool PopFront(T& out);
  
  size_t Size() const;
  bool Empty() const;
  void Clear();
  
  bool TryGet(size_t index, T& out) const;
  T operator[](size_t index) const;
  
  void ForEach(const std::function<bool(const T&)>& callback);
  std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const;
  std::vector<T> Slice(size_t start, size_t end) const;
};
```

**Usage:**
```cpp
Vector<int>::Ptr vec = std::make_shared<Vector<int>>();
vec->PushBack(1);
vec->PushBack(2);
vec->PushFront(0);  // [0, 1, 2]

int value;
if (vec->TryGet(1, value)) {
  std::cout << "Value at index 1: " << value << std::endl;
}

// Iterate
vec->ForEach([](const int& val) {
  std::cout << val << " ";
  return true;  // Continue iteration
});
```

#### Map<K, V>

Thread-safe ordered map (std::map based).

```cpp
template <typename K, typename V>
class Map {
 public:
  using Ptr = std::shared_ptr<Map<K, V>>;
  
  bool Insert(const K& key, const V& value);
  bool TryGet(const K& key, V& out) const;
  bool TryRemove(const K& key, V& out);
  bool Contains(const K& key) const;
  
  size_t Size() const;
  bool Empty() const;
  void Clear();
  
  void ForEach(const std::function<bool(const K&, const V&)>& callback) const;
  std::vector<std::pair<K, V>> ToVector() const;
  std::vector<K> Keys() const;
  std::vector<V> Values() const;
};
```

**Usage:**
```cpp
Map<std::string, int>::Ptr map = std::make_shared<Map<std::string, int>>();
map->Insert("one", 1);
map->Insert("two", 2);

int value;
if (map->TryGet("one", value)) {
  std::cout << "Value: " << value << std::endl;
}

// Iterate
map->ForEach([](const std::string& key, const int& val) {
  std::cout << key << " = " << val << std::endl;
  return true;
});
```

#### HashMap<K, V>

Thread-safe hash map (std::unordered_map based).

```cpp
template <typename K, typename V>
class HashMap {
 public:
  using Ptr = std::shared_ptr<HashMap<K, V>>;
  
  bool Insert(const K& key, const V& value);
  bool TryGet(const K& key, V& out) const;
  bool TryRemove(const K& key, V& out);
  bool Contains(const K& key) const;
  
  size_t Size() const;
  bool Empty() const;
  void Clear();
  
  void ForEach(const std::function<bool(const K&, const V&)>& callback) const;
  std::vector<std::pair<K, V>> ToVector() const;
  std::vector<K> Keys() const;
  std::vector<V> Values() const;
};
```

**Usage:**
```cpp
HashMap<int, std::string>::Ptr hash_map = std::make_shared<HashMap<int, std::string>>();
hash_map->Insert(1, "one");
hash_map->Insert(2, "two");
```

#### Queue<T>

Thread-safe queue with blocking dequeue.

```cpp
template <typename T>
class Queue {
 public:
  using Ptr = std::shared_ptr<Queue<T>>;
  
  void Enqueue(const T& value);
  void Enqueue(T&& value);
  
  bool Dequeue(T& out);           // Blocking
  bool TryDequeue(T& out);        // Non-blocking
  bool TryGet(T& out) const;
  
  size_t Size() const;
  bool Empty() const;
  void Clear();
  
  std::vector<T> ToVector() const;
  std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const;
};
```

**Usage:**
```cpp
Queue<std::string>::Ptr queue = std::make_shared<Queue<std::string>>();

// Producer
queue->Enqueue("task1");
queue->Enqueue("task2");

// Consumer (blocking)
std::string task;
queue->Dequeue(task);  // Blocks if empty

// Consumer (non-blocking)
if (queue->TryDequeue(task)) {
  // Process task
}
```

#### List<T>

Thread-safe doubly-linked list.

```cpp
template <typename T>
class List {
 public:
  using Ptr = std::shared_ptr<List<T>>;
  
  void PushBack(const T& value);
  void PushBack(T&& value);
  void PushFront(const T& value);
  void PushFront(T&& value);
  bool PopBack(T& out);
  bool PopFront(T& out);
  
  bool TryGet(size_t index, T& out) const;
  size_t Size() const;
  bool Empty() const;
  void Clear();
  
  void ForEach(const std::function<bool(const T&)>& callback);
  std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const;
  std::vector<T> Slice(size_t start, size_t end) const;
  std::vector<T> ToVector() const;
};
```

#### Stack<T>

Thread-safe LIFO stack.

```cpp
template <typename T>
class Stack {
 public:
  using Ptr = std::shared_ptr<Stack<T>>;
  
  void Push(const T& value);
  void Push(T&& value);
  bool Pop(T& out);
  bool TryPop(T& out);
  bool TryGet(T& out) const;
  
  size_t Size() const;
  bool Empty() const;
  void Clear();
  
  std::vector<T> ToVector() const;
  std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const;
};
```

#### Tree

Thread-safe property tree based on `boost::property_tree::ptree`.

```cpp
class Tree {
 public:
  using Ptr = std::shared_ptr<Tree>;
  
  void Put(const std::string& path, const boost::property_tree::ptree& value);
  boost::property_tree::ptree Get(const std::string& path) const;
  bool HasPath(const std::string& path) const;
  void Remove(const std::string& path);
  void Clear();
  
  void ForEach(const std::function<bool(const std::string&, const boost::property_tree::ptree&)>& callback);
};
```

**Usage:**
```cpp
Tree::Ptr tree = std::make_shared<Tree>();

boost::property_tree::ptree value;
value.put("name", "example");
value.put("value", 42);

tree->Put("config.data", value);
auto retrieved = tree->Get("config.data");
```

## Examples

See [examples/concurrent/](examples/concurrent/) for usage examples:

| Example | Description |
|---------|-------------|
| `mutex_example.cpp` | Mutex and lock guards usage |
| `event_example.cpp` | Event synchronization |
| `vector_example.cpp` | Thread-safe vector |
| `map_example.cpp` | Thread-safe ordered map |
| `hash_map_example.cpp` | Thread-safe hash map |
| `queue_example.cpp` | Producer-consumer queue |
| `list_example.cpp` | Thread-safe list |
| `stack_example.cpp` | Thread-safe stack |
| `tree_example.cpp` | Thread-safe property tree |
| `thread_example.cpp` | Thread lifecycle management |
| `thread_pool_example.cpp` | Thread pool task submission |

## Notes

1. **Thread Safety**: All methods are thread-safe unless documented otherwise
2. **Deadlock Prevention**: Mutex is recursive to prevent self-deadlock
3. **Exception Safety**: Operations are exception-safe
4. **Move Semantics**: All containers support move-only types
5. **Stop Tokens**: Thread and ThreadPool support C++20 stop tokens
6. **Ptr Type Alias**: All classes provide `Ptr = std::shared_ptr<Class>` for convenience

## Testing

### Run Tests

```bash
# Run all tests
bazel test //tests/concurrent:concurrent_test

# Run with verbose output
bazel test //tests/concurrent:concurrent_test --test_output=all

# Run specific test suite
bazel test //tests/concurrent:concurrent_test --test_filter=MutexTest.*
```

### Coverage Report

```bash
# Generate coverage report (header-only library, coverage measured via template instantiation)
bazel coverage //tests/concurrent:concurrent_test --combined_report=lcov

# View coverage report location
cat bazel-out/_coverage/_coverage_report.dat
```

### Test Report

Current status: **133 tests passed**

## License

MIT License
