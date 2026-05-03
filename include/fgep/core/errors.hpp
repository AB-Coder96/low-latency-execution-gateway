#pragma once

#include <cstdint>
#include <string_view>

namespace fgep {

// -----------------------------------------------------------------------------
// Core error codes
// -----------------------------------------------------------------------------
//
enum class ErrorCode : std::uint8_t {
    ok,

    invalid_argument,
    invalid_state,
    not_found,
    duplicate,
    parse_error,
    io_error,
    timeout,
    unsupported,
    internal_error
};

[[nodiscard]] constexpr bool is_ok(ErrorCode error) noexcept {
    return error == ErrorCode::ok;
}

[[nodiscard]] constexpr bool is_error(ErrorCode error) noexcept {
    return !is_ok(error);
}

[[nodiscard]] constexpr std::string_view to_string(ErrorCode error) noexcept {
    switch (error) {
        case ErrorCode::ok:
            return "ok";
        case ErrorCode::invalid_argument:
            return "invalid_argument";
        case ErrorCode::invalid_state:
            return "invalid_state";
        case ErrorCode::not_found:
            return "not_found";
        case ErrorCode::duplicate:
            return "duplicate";
        case ErrorCode::parse_error:
            return "parse_error";
        case ErrorCode::io_error:
            return "io_error";
        case ErrorCode::timeout:
            return "timeout";
        case ErrorCode::unsupported:
            return "unsupported";
        case ErrorCode::internal_error:
            return "internal_error";
    }

    return "unknown";
}

// -----------------------------------------------------------------------------
// Small result wrapper
// -----------------------------------------------------------------------------
//
// Result<T> is a lightweight alternative to exceptions for simple operations.
//
// Usage:
//   Result<Price> parsed_price{ErrorCode::ok, 1012500};
//   Result<Price> bad_price{ErrorCode::parse_error, 0};
//

template <typename T>
struct Result {
    ErrorCode error{ErrorCode::ok};
    T value{};

    [[nodiscard]] constexpr bool ok() const noexcept {
        return is_ok(error);
    }

    [[nodiscard]] constexpr bool failed() const noexcept {
        return is_error(error);
    }
};

} // namespace fgep