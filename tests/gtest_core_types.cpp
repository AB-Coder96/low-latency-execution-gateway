#include "fgep/core/types.hpp"

#include <gtest/gtest.h>

#include <string_view>

namespace {

TEST(CoreTypes, SideHelpersIdentifyBidAndAsk) {
    EXPECT_TRUE(fgep::is_bid(fgep::Side::bid));
    EXPECT_FALSE(fgep::is_bid(fgep::Side::ask));

    EXPECT_TRUE(fgep::is_ask(fgep::Side::ask));
    EXPECT_FALSE(fgep::is_ask(fgep::Side::bid));
}

TEST(CoreTypes, SideStringConversion) {
    EXPECT_EQ(fgep::to_string(fgep::Side::bid), std::string_view{"bid"});
    EXPECT_EQ(fgep::to_string(fgep::Side::ask), std::string_view{"ask"});
}

TEST(CoreTypes, OrderActionStringConversion) {
    EXPECT_EQ(
        fgep::to_string(fgep::OrderAction::new_order),
        std::string_view{"new_order"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::OrderAction::cancel),
        std::string_view{"cancel"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::OrderAction::replace),
        std::string_view{"replace"}
    );
}

TEST(CoreTypes, RejectReasonStringConversion) {
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::none),
        std::string_view{"none"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::unknown_symbol),
        std::string_view{"unknown_symbol"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::unknown_order),
        std::string_view{"unknown_order"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::duplicate_order_id),
        std::string_view{"duplicate_order_id"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::invalid_quantity),
        std::string_view{"invalid_quantity"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::invalid_price),
        std::string_view{"invalid_price"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::risk_rejected),
        std::string_view{"risk_rejected"}
    );
    EXPECT_EQ(
        fgep::to_string(fgep::RejectReason::invalid_state_transition),
        std::string_view{"invalid_state_transition"}
    );
}

TEST(CoreTypes, PriceValidationRejectsNonPositivePrices) {
    EXPECT_TRUE(fgep::is_valid_price(1));
    EXPECT_TRUE(fgep::is_valid_price(1'000'000));
    EXPECT_FALSE(fgep::is_valid_price(0));
    EXPECT_FALSE(fgep::is_valid_price(-1));
}

TEST(CoreTypes, QuantityValidationRejectsZero) {
    EXPECT_TRUE(fgep::is_valid_quantity(1));
    EXPECT_TRUE(fgep::is_valid_quantity(100));
    EXPECT_FALSE(fgep::is_valid_quantity(0));
}

TEST(CoreTypes, NotionalCalculationRejectsNegativePrice) {
    EXPECT_EQ(fgep::calculate_notional(100, 5), 500U);
    EXPECT_EQ(fgep::calculate_notional(250, 4), 1'000U);
    EXPECT_EQ(fgep::calculate_notional(0, 10), 0U);
    EXPECT_EQ(fgep::calculate_notional(-100, 10), 0U);
}

} // namespace