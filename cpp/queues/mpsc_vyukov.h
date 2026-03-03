#pragma once
#include <atomic>
#include <cstdint>
#include <utility>

namespace zl {

// Vyukov MPSC queue (multiple producers, single consumer).
// Producers push nodes; consumer pops nodes.
// This implementation stores T inside Node and manages node lifetime (new/delete).
template <typename T>
class MpscQueue {
  struct Node {
    std::atomic<Node*> next{nullptr};
    T value;
    explicit Node(const T& v) : value(v) {}
    explicit Node(T&& v) : value(std::move(v)) {}
  };

 public:
  MpscQueue() {
    stub_ = new Node(T{});
    head_.store(stub_, std::memory_order_relaxed);
    tail_ = stub_;
  }

  ~MpscQueue() {
    // Drain remaining nodes
    T tmp{};
    while (try_pop(tmp)) {}
    delete stub_;
  }

  bool try_push(const T& v) {
    Node* n = new Node(v);
    n->next.store(nullptr, std::memory_order_relaxed);
    Node* prev = head_.exchange(n, std::memory_order_acq_rel);
    prev->next.store(n, std::memory_order_release);
    return true;
  }

  bool try_pop(T& out) {
    Node* tail = tail_;
    Node* next = tail->next.load(std::memory_order_acquire);
    if (!next) return false;
    out = std::move(next->value);
    tail_ = next;
    delete tail;
    return true;
  }

 private:
  std::atomic<Node*> head_{nullptr};
  Node* tail_{nullptr};
  Node* stub_{nullptr};
};

} // namespace zl
