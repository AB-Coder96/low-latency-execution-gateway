#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace zl {

// A baseline queue: mutex + condition variable.
// try_push/try_pop are non-blocking; for blocking behavior we use push/pop in harness.
// This keeps the "QueueLike" interface consistent.
template <typename T>
class MutexQueue {
 public:
  bool try_push(const T& v) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      q_.push_back(v);
    }
    cv_.notify_one();
    return true;
  }

  bool try_pop(T& out) {
    std::lock_guard<std::mutex> lk(mu_);
    if (q_.empty()) return false;
    out = q_.front();
    q_.pop_front();
    return true;
  }

  // Blocking pop used by some scenarios.
  bool pop_blocking(T& out) {
    std::unique_lock<std::mutex> lk(mu_);
    cv_.wait(lk, [&]{ return !q_.empty() || closed_; });
    if (q_.empty()) return false;
    out = q_.front();
    q_.pop_front();
    return true;
  }

  void close() {
    {
      std::lock_guard<std::mutex> lk(mu_);
      closed_ = true;
    }
    cv_.notify_all();
  }

 private:
  std::mutex mu_;
  std::condition_variable cv_;
  std::deque<T> q_;
  bool closed_ = false;
};

} // namespace zl
