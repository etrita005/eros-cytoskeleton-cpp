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
│   ├── AutoResetEvent
│   └── ManualResetEvent
├── Threading
│   ├── Thread (jthread wrapper)
│   └── ThreadPool
└── Containers
    ├── Vector
    ├── List
    ├── Queue
    ├── Stack
    ├── Map
    ├── HashMap
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

## Components

### 1. Mutex & Synchronization

- **Mutex**: Recursive mutex wrapper
- **ReadWriteMutex**: Shared/exclusive lock
- **MutexLock/ReadLock/WriteLock**: RAII guards

### 2. Events

- **AutoResetEvent**: Auto-reset after wait, `Notify()` to signal
- **ManualResetEvent**: Manual reset required, `Notify()` to signal

Event API:
- `Notify()`: Signal the event
- `Reset()`: Reset the event state
- `Join()`: Wait for event (blocking)
- `Join(timeout)`: Wait with timeout
- `IsNotified()`: Check if event is signaled

### 3. Containers

All containers provide:
- Thread-safe operations
- `ForEach()` iteration
- `Filter()` and `Slice()` operations
- Move semantics support

### 4. Threading

- **Thread**: `std::jthread` wrapper with stop token support
- **ThreadPool**: Fixed-size thread pool with task submission

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

See [test_report.md](test_report.md) for detailed test results.

Current status: **133 tests passed**

## License

MIT License
