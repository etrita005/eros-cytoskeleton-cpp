# Concurrent Module Test Documentation

Test suite for Cytoskeleton Concurrent module using Google Test.

## Running Tests

```bash
bazel test //tests/concurrent:concurrent_test
bazel test //tests/concurrent:concurrent_test --test_output=all
bazel test //tests/concurrent:concurrent_test --test_filter=MutexTest.*
```

## Test Files

- `mutex_test.cpp` - Mutex and ReadWriteMutex tests
- `event_test.cpp` - AutoResetEvent and ManualResetEvent tests
- `vector_test.cpp` - Concurrent Vector tests
- `map_test.cpp` - Concurrent Map tests
- `hash_map_test.cpp` - Concurrent HashMap tests
- `queue_test.cpp` - Concurrent Queue tests
- `list_test.cpp` - Concurrent List tests
- `stack_test.cpp` - Concurrent Stack tests
- `tree_test.cpp` - Concurrent Tree tests
- `thread_test.cpp` - Thread wrapper tests
- `thread_pool_test.cpp` - ThreadPool tests
