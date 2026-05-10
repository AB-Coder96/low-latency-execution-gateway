#pragma once

#include "fgep/core/errors.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace fgep::venue {

using Mic = std::array<char, 4>;

[[nodiscard]] constexpr bool is_valid_mic_char(char value) noexcept {
    const auto byte = static_cast<unsigned char>(value);

    return (byte >= static_cast<unsigned char>('A')
            && byte <= static_cast<unsigned char>('Z'))
        || (byte >= static_cast<unsigned char>('0')
            && byte <= static_cast<unsigned char>('9'));
}

[[nodiscard]] constexpr bool is_valid_mic(const Mic& mic) noexcept {
    for (const char character : mic) {
        if (!is_valid_mic_char(character)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline Result<Mic> make_mic(std::string_view text) noexcept {
    Mic mic{};

    if (text.size() != mic.size()) {
        return {ErrorCode::invalid_argument, mic};
    }

    for (std::size_t index = 0; index < mic.size(); ++index) {
        const char character = text[index];

        if (!is_valid_mic_char(character)) {
            return {ErrorCode::invalid_argument, mic};
        }

        mic[index] = character;
    }

    return {ErrorCode::ok, mic};
}

struct MicHash {
    [[nodiscard]] std::size_t operator()(const Mic& mic) const noexcept {
        std::size_t hash = 0;

        for (const char character : mic) {
            hash = (hash * 131U)
                ^ static_cast<std::size_t>(
                    static_cast<unsigned char>(character)
                );
        }

        return hash;
    }
};

} // namespace fgep::venue