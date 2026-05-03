#include "fgep/core/types.hpp"

#include <cassert>
#include <string_view>

int main() {
    using namespace fgep;

    // Side helpers
    assert(is_bid(Side::bid));
    assert(!is_bid(Side::ask));

    assert(is_ask(Side::ask));
    assert(!is_ask(Side::bid));

    assert(to_string(Side::bid) == std::string_view{"bid"});
    assert(to_string(Side::ask) == std::string_view{"ask"});

    // OrderAction string conversion
    assert(to_string(OrderAction::new_order) == std::string_view{"new_order"});
    assert(to_string(OrderAction::cancel) == std::string_view{"cancel"});
    assert(to_string(OrderAction::replace) == std::string_view{"replace"});

    // RejectReason string conversion
    assert(to_string(RejectReason::none) == std::string_view{"none"});
    assert(to_string(RejectReason::unknown_symbol) == std::string_view{"unknown_symbol"});
    assert(to_string(RejectReason::unknown_order) == std::string_view{"unknown_order"});
    assert(to_string(RejectReason::duplicate_order_id) == std::string_view{"duplicate_order_id"});
    assert(to_string(RejectReason::invalid_quantity) == std::string_view{"invalid_quantity"});
    assert(to_string(RejectReason::invalid_price) == std::string_view{"invalid_price"});
    assert(to_string(RejectReason::risk_rejected) == std::string_view{"risk_rejected"});
    assert(
        to_string(RejectReason::invalid_state_transition)
        == std::string_view{"invalid_state_transition"}
    );

    // Price validation
    assert(is_valid_price(1));
    assert(is_valid_price(1000000));
    assert(!is_valid_price(0));
    assert(!is_valid_price(-1));

    // Quantity validation
    assert(is_valid_quantity(1));
    assert(is_valid_quantity(100));
    assert(!is_valid_quantity(0));

    // Notional calculation
    assert(calculate_notional(100, 5) == 500);
    assert(calculate_notional(250, 4) == 1000);
    assert(calculate_notional(0, 10) == 0);
    assert(calculate_notional(-100, 10) == 0);

    return 0;
}