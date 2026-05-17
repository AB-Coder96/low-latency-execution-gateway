#include "fgep/bench/order_candidate_generator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace fgep::bench {
namespace {

template <std::size_t Size>
[[nodiscard]] std::array<char, Size> fixed_ascii_from_text(
    std::string_view text
) noexcept {
    std::array<char, Size> value{};
    value.fill(' ');

    const auto copy_count = std::min(Size, text.size());

    for (std::size_t index = 0; index < copy_count; ++index) {
        value[index] = text[index];
    }

    return value;
}

[[nodiscard]] ouch::Symbol default_symbol() noexcept {
    return fixed_ascii_from_text<8>("AAPL");
}

[[nodiscard]] std::size_t clamp_hot_count(
    std::size_t hot_symbol_count,
    std::size_t symbol_count
) noexcept {
    if (symbol_count == 0) {
        return 0;
    }

    if (hot_symbol_count == 0) {
        return 1;
    }

    return std::min(hot_symbol_count, symbol_count);
}

[[nodiscard]] std::uint8_t clamp_percent(std::uint8_t percent) noexcept {
    return percent > 100U ? 100U : percent;
}

} // namespace

OrderCandidateGenerator::OrderCandidateGenerator()
    : OrderCandidateGenerator{OrderCandidateGeneratorConfig{}} {
}

OrderCandidateGenerator::OrderCandidateGenerator(
    OrderCandidateGeneratorConfig config
)
    : config_{std::move(config)},
      next_user_ref_num_{config_.first_user_ref_num} {
    if (config_.symbols.empty()) {
        config_.symbols.push_back(default_symbol());
    }

    if (config_.min_quantity == 0) {
        config_.min_quantity = 1;
    }

    if (config_.max_quantity < config_.min_quantity) {
        config_.max_quantity = config_.min_quantity;
    }

    config_.hot_symbol_count = clamp_hot_count(
        config_.hot_symbol_count,
        config_.symbols.size()
    );
    config_.hot_symbol_percent = clamp_percent(config_.hot_symbol_percent);
}

ouch::EnterOrderMessage OrderCandidateGenerator::next_enter_order() {
    const auto user_ref_num = next_user_ref_num_;

    ++next_user_ref_num_;
    ++generated_count_;

    return ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = choose_side(),
        .quantity = choose_quantity(),
        .symbol = choose_symbol(),
        .price = choose_price(),
        .time_in_force = ouch::TimeInForce::day,
        .display = ouch::Display::visible,
        .capacity = ouch::Capacity::agency,
        .intermarket_sweep_eligibility =
            ouch::IntermarketSweepEligibility::not_eligible,
        .cross_type = ouch::CrossType::continuous_market,
        .cl_ord_id = make_cl_ord_id(user_ref_num),
        .optional_appendage = {}
    };
}

std::size_t OrderCandidateGenerator::generated_count() const noexcept {
    return generated_count_;
}

ouch::UserRefNum OrderCandidateGenerator::next_user_ref_num() const noexcept {
    return next_user_ref_num_;
}

void OrderCandidateGenerator::reset() noexcept {
    next_user_ref_num_ = config_.first_user_ref_num;
    generated_count_ = 0;
}

const ouch::Symbol& OrderCandidateGenerator::choose_symbol() const noexcept {
    const auto symbol_count = config_.symbols.size();

    if (symbol_count == 1) {
        return config_.symbols.front();
    }

    const auto sequence_index = generated_count_;
    const auto bucket = static_cast<std::uint8_t>(sequence_index % 100U);

    if (bucket < config_.hot_symbol_percent) {
        const auto hot_index = sequence_index % config_.hot_symbol_count;
        return config_.symbols[hot_index];
    }

    const auto cold_count = symbol_count - config_.hot_symbol_count;

    if (cold_count == 0) {
        const auto hot_index = sequence_index % config_.hot_symbol_count;
        return config_.symbols[hot_index];
    }

    const auto cold_index = config_.hot_symbol_count
        + (sequence_index % cold_count);

    return config_.symbols[cold_index];
}

ouch::Quantity OrderCandidateGenerator::choose_quantity() const noexcept {
    const auto range = config_.max_quantity - config_.min_quantity + 1U;
    const auto offset = static_cast<ouch::Quantity>(generated_count_ % range);

    return config_.min_quantity + offset;
}

ouch::Price4 OrderCandidateGenerator::choose_price() const noexcept {
    const auto offset = static_cast<ouch::Price4>(generated_count_ % 11U);
    return config_.base_price + (offset * config_.price_step);
}

ouch::Side OrderCandidateGenerator::choose_side() const noexcept {
    if (!config_.alternate_sides) {
        return ouch::Side::buy;
    }

    return (generated_count_ % 2U) == 0U
        ? ouch::Side::buy
        : ouch::Side::sell;
}

ouch::ClOrdId OrderCandidateGenerator::make_cl_ord_id(
    ouch::UserRefNum user_ref_num
) const noexcept {
    auto value = fixed_ascii_from_text<14>("CID");

    auto number = user_ref_num;

    for (std::size_t reverse_index = 0; reverse_index < 10U; ++reverse_index) {
        const auto index = value.size() - 1U - reverse_index;
        const auto digit = static_cast<char>('0' + (number % 10U));

        value[index] = digit;
        number /= 10U;
    }

    return value;
}

} // namespace fgep::bench