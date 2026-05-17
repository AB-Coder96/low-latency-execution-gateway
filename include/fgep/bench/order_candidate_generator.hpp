#pragma once

#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fgep::bench {

struct OrderCandidateGeneratorConfig {
    std::vector<ouch::Symbol> symbols{};
    ouch::UserRefNum first_user_ref_num{1};
    ouch::Quantity min_quantity{100};
    ouch::Quantity max_quantity{1'000};
    ouch::Price4 base_price{1'000'000};
    ouch::Price4 price_step{100};
    std::size_t hot_symbol_count{1};
    std::uint8_t hot_symbol_percent{80};
    bool alternate_sides{true};
};

class OrderCandidateGenerator {
public:
    OrderCandidateGenerator();

    explicit OrderCandidateGenerator(OrderCandidateGeneratorConfig config);

    [[nodiscard]] ouch::EnterOrderMessage next_enter_order();

    [[nodiscard]] std::size_t generated_count() const noexcept;

    [[nodiscard]] ouch::UserRefNum next_user_ref_num() const noexcept;

    void reset() noexcept;

private:
    OrderCandidateGeneratorConfig config_{};
    ouch::UserRefNum next_user_ref_num_{1};
    std::size_t generated_count_{};

    [[nodiscard]] const ouch::Symbol& choose_symbol() const noexcept;
    [[nodiscard]] ouch::Quantity choose_quantity() const noexcept;
    [[nodiscard]] ouch::Price4 choose_price() const noexcept;
    [[nodiscard]] ouch::Side choose_side() const noexcept;
    [[nodiscard]] ouch::ClOrdId make_cl_ord_id(
        ouch::UserRefNum user_ref_num
    ) const noexcept;
};

} // namespace fgep::bench