#pragma once
#include <concepts>

namespace zl {

template <typename Q, typename T>
concept QueueLike = requires(Q q, const T& in, T& out) {
  { q.try_push(in) } -> std::same_as<bool>;
  { q.try_pop(out) } -> std::same_as<bool>;
};

} // namespace zl
