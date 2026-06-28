#pragma once

#include <cstddef>

#ifndef FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
#define FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS 0
#endif

namespace fgep::runtime {

class NoHeapGuard {
public:
    NoHeapGuard() noexcept;
    ~NoHeapGuard() noexcept;

    NoHeapGuard(const NoHeapGuard&) = delete;
    NoHeapGuard& operator=(const NoHeapGuard&) = delete;

    NoHeapGuard(NoHeapGuard&&) = delete;
    NoHeapGuard& operator=(NoHeapGuard&&) = delete;

    [[nodiscard]] std::size_t allocation_count() const noexcept;
    [[nodiscard]] std::size_t allocation_bytes() const noexcept;
    [[nodiscard]] bool observed_allocation() const noexcept;

private:
    std::size_t start_count_{};
    std::size_t start_bytes_{};
};

[[nodiscard]] bool no_heap_guard_enabled() noexcept;
[[nodiscard]] bool no_heap_guard_active() noexcept;
[[nodiscard]] std::size_t no_heap_allocation_count() noexcept;
[[nodiscard]] std::size_t no_heap_allocation_bytes() noexcept;

namespace detail {

void enter_no_heap_guard() noexcept;
void leave_no_heap_guard() noexcept;
void record_heap_allocation(std::size_t size) noexcept;

} // namespace detail

} // namespace fgep::runtime