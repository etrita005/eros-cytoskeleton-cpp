#pragma once

#include <algorithm>
#include <chrono>
#include <memory>

#include "cytoskeleton/concurrent/event.h"
#include "cytoskeleton/concurrent/list.h"
#include "cytoskeleton/concurrent/mutex.h"
#include "cytoskeleton/itc/message_queue/message.h"

namespace com {
namespace etrita {
namespace eros {
namespace cytos {
namespace itc {
namespace message_queue {

class MessageQueue {
 public:
  using Ptr = std::shared_ptr<MessageQueue>;
  using MessagePtr = Message::Ptr;

  MessageQueue() = default;
  ~MessageQueue() { Quit(); }

  MessageQueue(const MessageQueue&) = delete;
  MessageQueue& operator=(const MessageQueue&) = delete;
  MessageQueue(MessageQueue&&) = delete;
  MessageQueue& operator=(MessageQueue&&) = delete;

  void Clean() {
    concurrent::MutexLock lock(mutex_);
    messages_.Clear();
  }

  MessagePtr Next() {
    int64_t next_poll_timeout_ms = 0;

    while (true) {
      uint64_t now = GetCurrentTimeMs();
      MessagePtr message;

      {
        concurrent::MutexLock lock(mutex_);

        if (quit_) {
          Clean();
          return nullptr;
        }

        if (messages_.Empty()) {
          next_poll_timeout_ms = 1000;  // Wait 1 second if empty
        } else {
          // Peek at the first message without removing it
          MessagePtr front_msg;
          if (!messages_.TryGet(0, front_msg)) {
            next_poll_timeout_ms = 1000;
          } else {
            if (now < front_msg->GetWhen()) {
              next_poll_timeout_ms = static_cast<int64_t>(front_msg->GetWhen() - now);
            } else {
              // Message is ready to be processed
              messages_.PopFront(message);
              return message;
            }
          }
        }
      }

      if (next_poll_timeout_ms <= 0) {
        next_poll_timeout_ms = 1;
      }

      // Wait for new messages or timeout
      event_.Join(std::chrono::milliseconds(next_poll_timeout_ms));
    }

    return nullptr;
  }

  bool EnqueueMessage(MessagePtr message, uint64_t delay_ms = 0) {
    if (delay_ms != 0) {
      delay_ms += GetCurrentTimeMs();
    }

    message->SetWhen(delay_ms);

    {
      concurrent::MutexLock lock(mutex_);

      if (quit_) {
        return false;
      }

      // Insert message in sorted order (by when time)
      if (messages_.Empty()) {
        messages_.PushBack(message);
      } else {
        // Find the correct position to insert
        size_t index = 0;
        MessagePtr current;
        bool found = false;

        // Get the list as vector for easier manipulation
        std::vector<MessagePtr> temp_list = messages_.ToVector();

        for (size_t i = 0; i < temp_list.size(); ++i) {
          if (temp_list[i]->GetWhen() > delay_ms) {
            index = i;
            found = true;
            break;
          }
        }

        if (!found) {
          messages_.PushBack(message);
        } else {
          // Insert at the found position
          InsertAt(index, message);
        }
      }
    }

    event_.Notify();
    return true;
  }

  void Quit() {
    concurrent::MutexLock lock(mutex_);
    quit_ = true;
    event_.Notify();
  }

  bool IsQuit() const {
    concurrent::MutexLock lock(const_cast<concurrent::Mutex&>(mutex_));
    return quit_;
  }

 private:
  static uint64_t GetCurrentTimeMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
  }

  void InsertAt(size_t index, MessagePtr message) {
    // We need to rebuild the list with the new message inserted
    std::vector<MessagePtr> temp_list = messages_.ToVector();
    temp_list.insert(temp_list.begin() + static_cast<std::ptrdiff_t>(index), message);

    messages_.Clear();
    for (auto& msg : temp_list) {
      messages_.PushBack(msg);
    }
  }

 private:
  concurrent::List<MessagePtr> messages_;
  concurrent::ManualResetEvent event_;
  concurrent::Mutex mutex_;
  bool quit_{false};
};

}  // namespace message_queue
}  // namespace itc
}  // namespace cytos
}  // namespace eros
}  // namespace etrita
}  // namespace com
