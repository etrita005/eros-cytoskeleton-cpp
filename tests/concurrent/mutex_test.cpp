#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "cytoskeleton/concurrent/mutex.h"

using namespace com::etrita::eros::cytos::concurrent;

TEST(MutexTest, BasicLockUnlock) {
  Mutex mutex;
  mutex.Lock();
  mutex.Unlock();
  EXPECT_TRUE(true);
}

TEST(MutexTest, TryLock) {
  Mutex mutex;
  EXPECT_TRUE(mutex.TryLock());
  mutex.Unlock();
}

TEST(MutexTest, RecursiveLock) {
  Mutex mutex;
  mutex.Lock();
  mutex.Lock();
  mutex.Unlock();
  mutex.Unlock();
  EXPECT_TRUE(true);
}

TEST(MutexLockTest, RAII) {
  Mutex mutex;
  {
    MutexLock lock(mutex);
  }
  EXPECT_TRUE(mutex.TryLock());
  mutex.Unlock();
}

TEST(ReadWriteMutexTest, BasicReadLock) {
  ReadWriteMutex mutex;
  mutex.LockRead();
  mutex.UnlockRead();
  EXPECT_TRUE(true);
}

TEST(ReadWriteMutexTest, BasicWriteLock) {
  ReadWriteMutex mutex;
  mutex.LockWrite();
  mutex.UnlockWrite();
  EXPECT_TRUE(true);
}

TEST(ReadWriteMutexTest, TryLockRead) {
  ReadWriteMutex mutex;
  EXPECT_TRUE(mutex.TryLockRead());
  mutex.UnlockRead();
}

TEST(ReadWriteMutexTest, TryLockWrite) {
  ReadWriteMutex mutex;
  EXPECT_TRUE(mutex.TryLockWrite());
  mutex.UnlockWrite();
}

TEST(ReadLockTest, RAII) {
  ReadWriteMutex mutex;
  {
    ReadLock lock(mutex);
  }
  EXPECT_TRUE(mutex.TryLockWrite());
  mutex.UnlockWrite();
}

TEST(WriteLockTest, RAII) {
  ReadWriteMutex mutex;
  {
    WriteLock lock(mutex);
  }
  EXPECT_TRUE(mutex.TryLockRead());
  mutex.UnlockRead();
}

TEST(MutexTest, ConcurrentAccess) {
  Mutex mutex;
  int counter = 0;
  const int kNumThreads = 10;
  const int kIncrementsPerThread = 1000;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < kIncrementsPerThread; ++j) {
        MutexLock lock(mutex);
        ++counter;
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(counter, kNumThreads * kIncrementsPerThread);
}

TEST(ReadWriteMutexTest, ConcurrentReadAccess) {
  ReadWriteMutex mutex;
  int counter = 0;
  const int kNumThreads = 10;
  const int kIncrementsPerThread = 1000;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < kIncrementsPerThread; ++j) {
        WriteLock lock(mutex);
        ++counter;
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(counter, kNumThreads * kIncrementsPerThread);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
