#include "fgep/bench/order_candidate_generator.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

} // namespace

int main() {
    using namespace fgep::bench;

    {
        OrderCandidateGenerator generator{};

        const auto first = generator.next_enter_order();
        const auto second = generator.next_enter_order();

        assert(first.user_ref_num == 1);
        assert(second.user_ref_num == 2);
        assert(first.side == fgep::ouch::Side::sell);
        assert(second.side == fgep::ouch::Side::buy);
        assert(first.quantity == 101);
        assert(second.quantity == 102);
        assert(first.price == 1'000'100);
        assert(second.price == 1'000'200);
        assert(generator.generated_count() == 2);
        assert(generator.next_user_ref_num() == 3);
    }

    {
        OrderCandidateGenerator generator{OrderCandidateGeneratorConfig{
            .symbols = {symbol("AAPL"), symbol("MSFT"), symbol("TSLA")},
            .first_user_ref_num = 100,
            .min_quantity = 10,
            .max_quantity = 12,
            .base_price = 2'000'000,
            .price_step = 25,
            .hot_symbol_count = 2,
            .hot_symbol_percent = 100,
            .alternate_sides = false
        }};

        const auto first = generator.next_enter_order();
        const auto second = generator.next_enter_order();
        const auto third = generator.next_enter_order();
        const auto fourth = generator.next_enter_order();

        assert(first.user_ref_num == 100);
        assert(second.user_ref_num == 101);
        assert(third.user_ref_num == 102);
        assert(fourth.user_ref_num == 103);

        assert(first.side == fgep::ouch::Side::buy);
        assert(second.side == fgep::ouch::Side::buy);

        assert(first.quantity == 11);
        assert(second.quantity == 12);
        assert(third.quantity == 10);
        assert(fourth.quantity == 11);

        assert(first.price == 2'000'025);
        assert(second.price == 2'000'050);
        assert(third.price == 2'000'075);
        assert(fourth.price == 2'000'100);

        assert(first.symbol == symbol("MSFT"));
        assert(second.symbol == symbol("AAPL"));
        assert(third.symbol == symbol("MSFT"));
        assert(fourth.symbol == symbol("AAPL"));
    }

    {
        OrderCandidateGenerator generator{OrderCandidateGeneratorConfig{
            .symbols = {symbol("AAPL"), symbol("MSFT"), symbol("TSLA")},
            .first_user_ref_num = 1,
            .min_quantity = 1,
            .max_quantity = 1,
            .base_price = 100,
            .price_step = 1,
            .hot_symbol_count = 1,
            .hot_symbol_percent = 0,
            .alternate_sides = true
        }};

        const auto first = generator.next_enter_order();
        const auto second = generator.next_enter_order();
        const auto third = generator.next_enter_order();

        assert(first.symbol == symbol("TSLA"));
        assert(second.symbol == symbol("MSFT"));
        assert(third.symbol == symbol("TSLA"));
    }

    {
        OrderCandidateGenerator generator{OrderCandidateGeneratorConfig{
            .symbols = {symbol("AAPL")},
            .first_user_ref_num = 50,
            .min_quantity = 0,
            .max_quantity = 0,
            .base_price = 100,
            .price_step = 0,
            .hot_symbol_count = 0,
            .hot_symbol_percent = 200,
            .alternate_sides = true
        }};

        const auto order = generator.next_enter_order();

        assert(order.user_ref_num == 50);
        assert(order.quantity == 1);
        assert(order.symbol == symbol("AAPL"));

        generator.reset();

        const auto reset_order = generator.next_enter_order();

        assert(reset_order.user_ref_num == 50);
        assert(generator.generated_count() == 1);
        assert(generator.next_user_ref_num() == 51);
    }

    return 0;
}