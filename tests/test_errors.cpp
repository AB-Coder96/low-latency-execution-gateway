#include "fgep/core/errors.hpp"

#include <cassert>
#include <string_view>

int main() {
    using namespace fgep;

    // Basic success/failure helpers
    assert(is_ok(ErrorCode::ok));
    assert(!is_error(ErrorCode::ok));

    assert(!is_ok(ErrorCode::invalid_argument));
    assert(is_error(ErrorCode::invalid_argument));

    // String conversion
    assert(to_string(ErrorCode::ok) == std::string_view{"ok"});
    assert(to_string(ErrorCode::invalid_argument) == std::string_view{"invalid_argument"});
    assert(to_string(ErrorCode::invalid_state) == std::string_view{"invalid_state"});
    assert(to_string(ErrorCode::not_found) == std::string_view{"not_found"});
    assert(to_string(ErrorCode::duplicate) == std::string_view{"duplicate"});
    assert(to_string(ErrorCode::parse_error) == std::string_view{"parse_error"});
    assert(to_string(ErrorCode::io_error) == std::string_view{"io_error"});
    assert(to_string(ErrorCode::timeout) == std::string_view{"timeout"});
    assert(to_string(ErrorCode::unsupported) == std::string_view{"unsupported"});
    assert(to_string(ErrorCode::internal_error) == std::string_view{"internal_error"});

    // Result<T> success case
    Result<int> good_result{
        ErrorCode::ok,
        42
    };

    assert(good_result.ok());
    assert(!good_result.failed());
    assert(good_result.value == 42);

    // Result<T> failure case
    Result<int> bad_result{
        ErrorCode::parse_error,
        0
    };

    assert(!bad_result.ok());
    assert(bad_result.failed());
    assert(bad_result.error == ErrorCode::parse_error);
    assert(bad_result.value == 0);

    return 0;
}