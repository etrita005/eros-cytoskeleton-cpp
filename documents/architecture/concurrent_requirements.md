# Cytoskeleton C++ 基础库 - Concurrent 模块需求文档

## 1. 概述

### 1.1 模块名称
`concurrent` - 并发处理模块

### 1.2 命名空间
`com::etrita::eros::cytos::concurrent`

### 1.3 设计目标
- 提供线程安全的数据结构封装
- 提供常用的同步原语
- 提供线程和线程池的便捷封装
- API 风格参考 C# 和 Android，遵循 Google C++ Style Guide
- **面向机器人应用与服务开发，不面向高性能场景**
- **API 设计原则：安全性 > 易用性 > 性能**

### 1.4 C++ 标准
C++20

### 1.5 命名约定
- 类名：大驼峰命名（PascalCase），如 `Vector`、`Map`
- 方法名：大驼峰命名（PascalCase），如 `PushBack`、`TryGet`
- 参数名：小驼峰命名（camelCase），如 `pool_size`

---

## 2. 线程安全容器

**实现说明：**
- 所有线程安全容器内部使用 `std::mutex` 实现线程安全
- 读多写少的场景（如 `Map`、`HashMap`）使用 `std::shared_mutex`
- 写多读少的场景（如 `Queue`、`Stack`）使用 `std::mutex`
- **重要**：`Queue` 和 `Stack` 使用 `std::condition_variable` 支持阻塞等待

### 2.1 Vector<T>

线程安全的动态数组。

**类型定义：**
- `using Ptr = std::shared_ptr<Vector<T>>` - Vector 智能指针类型

**API:**
- `void PushBack(const T& value)` / `void PushBack(T&& value)`
- `void PushFront(const T& value)` / `void PushFront(T&& value)`
- `bool PopBack(T& out)` / `bool PopFront(T& out)`
- `size_t Size() const`
- `bool Empty() const`
- `void Clear()`
- `bool TryGet(size_t index, T& out) const` - 按索引获取元素
- `T operator[](size_t index) const` - 下标访问（返回副本）
- `void ForEach(const std::function<bool(const T&)>& callback)` - 锁内执行回调
- `std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const` - 过滤
- `std::vector<T> Slice(size_t start, size_t end) const` - 切片

### 2.2 Map<K, V>

线程安全的有序 map（基于 `std::map`）。

**类型定义：**
- `using Ptr = std::shared_ptr<Map<K, V>>` - Map 智能指针类型

**API:**
- `bool Insert(const K& key, const V& value)` / `bool Insert(K&& key, V&& value)`
- `bool TryGet(const K& key, V& out) const`
- `bool TryRemove(const K& key, V& out)`
- `bool Contains(const K& key) const`
- `size_t Size() const`
- `bool Empty() const`
- `void Clear()`
- `void ForEach(const std::function<bool(const K&, const V&)>& callback)`
- `std::vector<std::pair<K, V>> ToVector() const` - 转换为 vector
- `std::vector<K> Keys() const` - 获取所有键
- `std::vector<V> Values() const` - 获取所有值

### 2.3 HashMap<K, V>

线程安全的无序 map（基于 `std::unordered_map`）。

**类型定义：**
- `using Ptr = std::shared_ptr<HashMap<K, V>>` - HashMap 智能指针类型

**API:**
- `bool Insert(const K& key, const V& value)` / `bool Insert(K&& key, V&& value)`
- `bool TryGet(const K& key, V& out) const`
- `bool TryRemove(const K& key, V& out)`
- `bool Contains(const K& key) const`
- `size_t Size() const`
- `bool Empty() const`
- `void Clear()`
- `void ForEach(const std::function<bool(const K&, const V&)>& callback)`
- `std::vector<std::pair<K, V>> ToVector() const` - 转换为 vector
- `std::vector<K> Keys() const` - 获取所有键
- `std::vector<V> Values() const` - 获取所有值

### 2.4 Queue<T>

线程安全的 FIFO 队列，支持阻塞等待。

**类型定义：**
- `using Ptr = std::shared_ptr<Queue<T>>` - Queue 智能指针类型

**API:**
- `void Enqueue(const T& value)` / `void Enqueue(T&& value)` - 入队，入队后通知等待的消费者
- `bool Dequeue(T& out)` - **阻塞**出队，队列为空时阻塞等待
- `bool TryDequeue(T& out)` - **非阻塞**出队，队列为空时返回 false
- `bool TryGet(T& out) const` - 获取队首元素但不移除
- `size_t Size() const`
- `bool Empty() const`
- `void Clear()`
- `std::vector<T> ToVector() const` - 转换为 vector
- `std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const` - 过滤

**线程安全说明：**
- 使用 `std::mutex` 保护内部数据
- 使用 `std::condition_variable` 实现阻塞等待
- `Enqueue` 操作会通知一个等待的 `Dequeue`

### 2.5 List<T>

线程安全的双向链表（基于 `std::list`）。

**类型定义：**
- `using Ptr = std::shared_ptr<List<T>>` - List 智能指针类型

**API:**
- `void PushBack(const T& value)` / `void PushBack(T&& value)`
- `void PushFront(const T& value)` / `void PushFront(T&& value)`
- `bool PopBack(T& out)` / `bool PopFront(T& out)`
- `bool TryGet(size_t index, T& out) const` - 按索引获取元素
- `size_t Size() const`
- `bool Empty() const`
- `void Clear()`
- `void ForEach(const std::function<bool(const T&)>& callback)`
- `std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const` - 过滤
- `std::vector<T> Slice(size_t start, size_t end) const` - 切片
- `std::vector<T> ToVector() const` - 转换为 vector

### 2.6 Stack<T>

线程安全的栈（LIFO），支持阻塞等待。

**类型定义：**
- `using Ptr = std::shared_ptr<Stack<T>>` - Stack 智能指针类型

**API:**
- `void Push(const T& value)` / `void Push(T&& value)` - 入栈，入栈后通知等待的消费者
- `bool Pop(T& out)` - **阻塞**出栈，栈为空时阻塞等待
- `bool TryPop(T& out)` - **非阻塞**出栈，栈为空时返回 false
- `bool TryGet(T& out) const` - 获取栈顶元素但不移除
- `size_t Size() const`
- `bool Empty() const`
- `void Clear()`
- `std::vector<T> ToVector() const` - 转换为 vector
- `std::vector<T> Filter(const std::function<bool(const T&)>& predicate) const` - 过滤

**线程安全说明：**
- 使用 `std::mutex` 保护内部数据
- 使用 `std::condition_variable` 实现阻塞等待
- `Push` 操作会通知一个等待的 `Pop`

### 2.7 Tree

线程安全的树状结构封装（基于 `boost::property_tree`）。

**类型定义：**
- `using Ptr = std::shared_ptr<Tree>` - Tree 智能指针类型

**API:**
- `void Put(const std::string& path, const boost::property_tree::ptree& value)`
- `boost::property_tree::ptree Get(const std::string& path) const`
- `bool HasPath(const std::string& path) const`
- `void Remove(const std::string& path)`
- `void Clear()`
- `void ForEach(const std::function<bool(const std::string&, const boost::property_tree::ptree&)>& callback)`

---

## 3. 同步原语

### 3.1 Event

事件同步基类。

**实现说明：**
- `Join()` 和 `Join(timeout)` 必须正确处理 spurious wakeup（虚假唤醒）
- 使用 `std::condition_variable::wait_for()` 配合循环检查条件实现

**类型定义：**
- `using Ptr = std::shared_ptr<Event>` - Event 智能指针类型

**API:**
- `explicit Event(bool initial_state = false)`
- `virtual ~Event()`
- `void Set()` - 设置事件为有信号状态
- `virtual void Reset() = 0` - 重置事件为无信号状态（纯虚函数）
- `void Join()` - 等待事件
- `bool Join(std::chrono::milliseconds timeout)` - 超时等待，返回是否成功（事件被设置）
- `bool IsSet() const` - 查询当前状态

### 3.2 AutoResetEvent

自动重置事件（子类）。

**行为:** `Join()` 成功后自动调用 `Reset()`

**类型定义：**
- `using Ptr = std::shared_ptr<AutoResetEvent>` - AutoResetEvent 智能指针类型

**API:**
- `explicit AutoResetEvent(bool initial_state = false)`
- `void Reset() override`

### 3.3 ManualResetEvent

手动重置事件（子类）。

**行为:** `Join()` 成功后保持有信号状态，需手动调用 `Reset()`

**类型定义：**
- `using Ptr = std::shared_ptr<ManualResetEvent>` - ManualResetEvent 智能指针类型

**API:**
- `explicit ManualResetEvent(bool initial_state = false)`
- `void Reset() override`

### 3.4 Mutex

互斥锁包装（基于 `std::recursive_mutex`，支持嵌套加锁）。

**类型定义：**
- `using Ptr = std::shared_ptr<Mutex>` - Mutex 智能指针类型

**API:**
- `void Lock()`
- `void Unlock()`
- `bool TryLock()`

### 3.5 ReadWriteMutex

读写锁包装（基于 `std::shared_mutex`）。

**类型定义：**
- `using Ptr = std::shared_ptr<ReadWriteMutex>` - ReadWriteMutex 智能指针类型

**API:**
- `void LockRead()`
- `void UnlockRead()`
- `void LockWrite()`
- `void UnlockWrite()`
- `bool TryLockRead()`
- `bool TryLockWrite()`

### 3.6 MutexLock

互斥锁 RAII 守卫。

**类型定义：**
- `using Ptr = std::shared_ptr<MutexLock>` - MutexLock 智能指针类型

**API:**
- `explicit MutexLock(Mutex& mutex)`
- `~MutexLock()`

### 3.7 ReadLock

读锁 RAII 守卫。

**类型定义：**
- `using Ptr = std::shared_ptr<ReadLock>` - ReadLock 智能指针类型

**API:**
- `explicit ReadLock(ReadWriteMutex& mutex)`
- `~ReadLock()`

### 3.8 WriteLock

写锁 RAII 守卫。

**类型定义：**
- `using Ptr = std::shared_ptr<WriteLock>` - WriteLock 智能指针类型

**API:**
- `explicit WriteLock(ReadWriteMutex& mutex)`
- `~WriteLock()`

---

## 4. 线程相关

### 4.1 Thread

线程封装类（内部基于 `std::jthread` 实现）。

**设计说明：**
- **不使用虚函数**，避免 vptr 数据竞争问题
- 完全使用 `std::function` 回调模式
- 析构时自动调用 `RequestStop()` 和 `Join()`

**构造方式 - Lambda/函数:**
```cpp
Thread thread("my_thread", [](std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    // 执行任务
  }
});
```

**类型定义：**
- `using Ptr = std::shared_ptr<Thread>` - Thread 智能指针类型

**API:**
- `explicit Thread(const std::string& name)` - 默认构造（空函数）
- `Thread(const std::string& name, std::function<void(std::stop_token)> func)` - 传入函数/lambda
- `~Thread()` - 析构时自动 `RequestStop()` 和 `Join()`
- `void Start()` - 启动线程（使用原子操作保证只启动一次）
- `void Join()` - 等待线程结束
- `bool Join(std::chrono::milliseconds timeout)` - 超时等待
- `void RequestStop()` - 请求线程停止
- `bool ShouldStop() const` - 查询是否应该停止
- `std::string GetName() const` - 获取线程名称

**线程安全说明：**
- 使用 `std::atomic<bool>` 保护 `started_` 标志
- Lambda 捕获 `name` 和 `func` 的副本，避免 `this` 指针问题
- 析构函数先 `RequestStop()` 再 `Join()`，确保安全退出

### 4.2 ThreadPool

线程池封装（固定大小）。

**构造方式:**
```cpp
ThreadPool pool(4);  // 4 个线程
```

**类型定义：**
- `using Ptr = std::shared_ptr<ThreadPool>` - ThreadPool 智能指针类型

**API:**
- `explicit ThreadPool(size_t pool_size)`
- `~ThreadPool()` - 析构时自动 Shutdown
- `template<typename F, typename... Args> auto Submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>`
- `void Shutdown()` - 优雅关闭（等待所有任务完成）
- `void Join()` - 等待所有任务完成

---

## 5. 依赖

- C++20 标准库
- Boost (用于 `Tree`)
  - `boost::property_tree`

---

## 6. 测试要求

- 所有线程安全容器需要进行多线程并发测试
- 同步原语需要进行边界条件测试
- `Thread` 类需要测试析构时自动 Join 行为
- `ThreadPool` 需要测试动态伸缩行为
- 使用 ThreadSanitizer 检测数据竞争
