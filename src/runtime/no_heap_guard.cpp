#include "fgep/runtime/no_heap_guard.hpp"

#include <cstdlib>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace fgep::runtime {
namespace {

thread_local std::size_t guard_depth = 0U;
thread_local std::size_t allocation_count = 0U;
thread_local std::size_t allocation_bytes = 0U;

} // namespace

NoHeapGuard::NoHeapGuard() noexcept
    : start_count_{no_heap_allocation_count()},
      start_bytes_{no_heap_allocation_bytes()} {
    detail::enter_no_heap_guard();
}

NoHeapGuard::~NoHeapGuard() noexcept {
    detail::leave_no_heap_guard();
}

std::size_t NoHeapGuard::allocation_count() const noexcept {
    return no_heap_allocation_count() - start_count_;
}

std::size_t NoHeapGuard::allocation_bytes() const noexcept {
    return no_heap_allocation_bytes() - start_bytes_;
}

bool NoHeapGuard::observed_allocation() const noexcept {
    return allocation_count() != 0U;
}

bool no_heap_guard_enabled() noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    return true;
#else
    return false;
#endif
}

bool no_heap_guard_active() noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    return guard_depth != 0U;
#else
    return false;
#endif
}

std::size_t no_heap_allocation_count() noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    return allocation_count;
#else
    return 0U;
#endif
}

std::size_t no_heap_allocation_bytes() noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    return allocation_bytes;
#else
    return 0U;
#endif
}

void detail::enter_no_heap_guard() noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    ++guard_depth;
#endif
}

void detail::leave_no_heap_guard() noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    if (guard_depth != 0U) {
        --guard_depth;
    }
#endif
}

void detail::record_heap_allocation(std::size_t size) noexcept {
#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    if (guard_depth != 0U) {
        ++allocation_count;
        allocation_bytes += size;
    }
#else
    (void)size;
#endif
}

} // namespace fgep::runtime

#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS

namespace {

[[nodiscard]] std::size_t normalized_allocation_size(
    std::size_t size
) noexcept {
    return size == 0U ? 1U : size;
}

[[nodiscard]] void* allocate_heap(std::size_t size) {
    const auto normalized_size = normalized_allocation_size(size);

    fgep::runtime::detail::record_heap_allocation(normalized_size);

    if (void* pointer = std::malloc(normalized_size)) {
        return pointer;
    }

    throw std::bad_alloc{};
}

[[nodiscard]] void* allocate_heap_nothrow(std::size_t size) noexcept {
    const auto normalized_size = normalized_allocation_size(size);

    fgep::runtime::detail::record_heap_allocation(normalized_size);

    return std::malloc(normalized_size);
}

[[nodiscard]] void* allocate_aligned_heap(
    std::size_t size,
    std::size_t alignment
) {
    const auto normalized_size = normalized_allocation_size(size);

    fgep::runtime::detail::record_heap_allocation(normalized_size);

#if defined(_WIN32)
    if (void* pointer = _aligned_malloc(normalized_size, alignment)) {
        return pointer;
    }
#else
    void* pointer = nullptr;

    if (posix_memalign(&pointer, alignment, normalized_size) == 0) {
        return pointer;
    }
#endif

    throw std::bad_alloc{};
}

[[nodiscard]] void* allocate_aligned_heap_nothrow(
    std::size_t size,
    std::size_t alignment
) noexcept {
    const auto normalized_size = normalized_allocation_size(size);

    fgep::runtime::detail::record_heap_allocation(normalized_size);

#if defined(_WIN32)
    return _aligned_malloc(normalized_size, alignment);
#else
    void* pointer = nullptr;

    if (posix_memalign(&pointer, alignment, normalized_size) != 0) {
        return nullptr;
    }

    return pointer;
#endif
}

void free_aligned_heap(void* pointer) noexcept {
#if defined(_WIN32)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

} // namespace

void* operator new(std::size_t size) {
    return allocate_heap(size);
}

void* operator new[](std::size_t size) {
    return allocate_heap(size);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return allocate_heap_nothrow(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return allocate_heap_nothrow(size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_aligned_heap(
        size,
        static_cast<std::size_t>(alignment)
    );
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_aligned_heap(
        size,
        static_cast<std::size_t>(alignment)
    );
}

void* operator new(
    std::size_t size,
    std::align_val_t alignment,
    const std::nothrow_t&
) noexcept {
    return allocate_aligned_heap_nothrow(
        size,
        static_cast<std::size_t>(alignment)
    );
}

void* operator new[](
    std::size_t size,
    std::align_val_t alignment,
    const std::nothrow_t&
) noexcept {
    return allocate_aligned_heap_nothrow(
        size,
        static_cast<std::size_t>(alignment)
    );
}

void operator delete(void* pointer) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, const std::nothrow_t&) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t&) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::align_val_t) noexcept {
    free_aligned_heap(pointer);
}

void operator delete[](void* pointer, std::align_val_t) noexcept {
    free_aligned_heap(pointer);
}

void operator delete(
    void* pointer,
    std::size_t,
    std::align_val_t
) noexcept {
    free_aligned_heap(pointer);
}

void operator delete[](
    void* pointer,
    std::size_t,
    std::align_val_t
) noexcept {
    free_aligned_heap(pointer);
}

void operator delete(
    void* pointer,
    std::align_val_t,
    const std::nothrow_t&
) noexcept {
    free_aligned_heap(pointer);
}

void operator delete[](
    void* pointer,
    std::align_val_t,
    const std::nothrow_t&
) noexcept {
    free_aligned_heap(pointer);
}

#endif