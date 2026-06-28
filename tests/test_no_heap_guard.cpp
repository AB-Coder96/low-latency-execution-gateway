#include "fgep/runtime/no_heap_guard.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <new>
#include <vector>

int main() {
    using namespace fgep::runtime;

#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    assert(no_heap_guard_enabled());
#else
    assert(!no_heap_guard_enabled());
#endif

    assert(!no_heap_guard_active());

    {
        NoHeapGuard guard{};

#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
        assert(no_heap_guard_active());
#endif

        std::array<std::byte, 128> buffer{};

        for (std::size_t index = 0; index < buffer.size(); ++index) {
            buffer[index] = static_cast<std::byte>(index & 0xFFU);
        }

        assert(!guard.observed_allocation());
        assert(guard.allocation_count() == 0U);
        assert(guard.allocation_bytes() == 0U);
    }

    assert(!no_heap_guard_active());

#if FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS
    {
        NoHeapGuard guard{};

        auto* value = new int{42};

        assert(value != nullptr);
        assert(*value == 42);
        assert(guard.observed_allocation());
        assert(guard.allocation_count() >= 1U);
        assert(guard.allocation_bytes() >= sizeof(int));

        delete value;
    }

    {
        NoHeapGuard guard{};

        std::vector<int> values{};
        values.push_back(1);
        values.push_back(2);
        values.push_back(3);

        assert(values.size() == 3U);
        assert(guard.observed_allocation());
        assert(guard.allocation_count() >= 1U);
    }

    {
        NoHeapGuard outer{};

        {
            NoHeapGuard inner{};

            auto* value = new int{7};

            assert(value != nullptr);
            assert(*value == 7);
            assert(inner.observed_allocation());

            delete value;
        }

        assert(outer.observed_allocation());
    }
#endif

    return 0;
}