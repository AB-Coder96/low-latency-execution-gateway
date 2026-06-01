#include "fgep/execution/encoded_backend_submit.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/moldudp64/moldudp64_decode.hpp"
#include "fgep/moldudp64/moldudp64_types.hpp"
#include "fgep/ouch/ouch_decode.hpp"
#include "fgep/ouch/ouch_encode.hpp"
#include "fgep/ouch/ouch_types.hpp"
#include "fgep/wire/byte_io.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <span>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
}

[[nodiscard]] fgep::ouch::ClOrdId cl_ord_id(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<14>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
}

[[nodiscard]] fgep::ouch::EnterOrderMessage enter_order(
    fgep::ouch::UserRefNum user_ref_num
) {
    return fgep::ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = fgep::ouch::Side::buy,
        .quantity = 100,
        .symbol = symbol("AAPL"),
        .price = 1'902'500,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .capacity = fgep::ouch::Capacity::agency,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cross_type = fgep::ouch::CrossType::continuous_market,
        .cl_ord_id = cl_ord_id("ORDER1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::ReplaceOrderMessage replace_order(
    fgep::ouch::UserRefNum original_user_ref_num,
    fgep::ouch::UserRefNum replacement_user_ref_num
) {
    return fgep::ouch::ReplaceOrderMessage{
        .orig_user_ref_num = original_user_ref_num,
        .user_ref_num = replacement_user_ref_num,
        .quantity = 150,
        .price = 1'903'000,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cl_ord_id = cl_ord_id("REPLACE1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::CancelOrderMessage cancel_order(
    fgep::ouch::UserRefNum user_ref_num
) {
    return fgep::ouch::CancelOrderMessage{
        .user_ref_num = user_ref_num,
        .quantity = 0,
        .optional_appendage = {}
    };
}

[[nodiscard]] std::array<std::byte, fgep::ouch::length_enter_order_base>
encoded_enter_order() {
    std::array<std::byte, fgep::ouch::length_enter_order_base> bytes{};
    const auto error = fgep::ouch::encode_enter_order_message(
        std::span<std::byte>{bytes.data(), bytes.size()},
        enter_order(1)
    );

    benchmark::DoNotOptimize(error);
    return bytes;
}

[[nodiscard]] std::array<std::byte, fgep::moldudp64::length_downstream_header>
heartbeat_packet() {
    std::array<std::byte, fgep::moldudp64::length_downstream_header> bytes{};

    for (std::size_t index = 0; index < fgep::moldudp64::length_session; ++index) {
        bytes[index] = static_cast<std::byte>('A' + static_cast<int>(index));
    }

    benchmark::DoNotOptimize(
        fgep::wire::write_u64_be(
            std::span<std::byte>{bytes.data(), bytes.size()},
            fgep::moldudp64::offset_sequence_number,
            1
        )
    );

    benchmark::DoNotOptimize(
        fgep::wire::write_u16_be(
            std::span<std::byte>{bytes.data(), bytes.size()},
            fgep::moldudp64::offset_message_count,
            fgep::moldudp64::heartbeat_message_count
        )
    );

    return bytes;
}

void BM_OuchEncodeEnterOrder(benchmark::State& state) {
    std::array<std::byte, fgep::ouch::length_enter_order_base> bytes{};
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        const auto message = enter_order(user_ref_num++);

        const auto error = fgep::ouch::encode_enter_order_message(
            std::span<std::byte>{bytes.data(), bytes.size()},
            message
        );

        benchmark::DoNotOptimize(error);
        benchmark::ClobberMemory();
    }
}

void BM_OuchDecodeEnterOrder(benchmark::State& state) {
    const auto bytes = encoded_enter_order();

    for (auto _ : state) {
        const auto result = fgep::ouch::decode_enter_order_message(
            std::span<const std::byte>{bytes.data(), bytes.size()}
        );

        benchmark::DoNotOptimize(result);
    }
}

void BM_MoldUdp64DecodeHeartbeat(benchmark::State& state) {
    const auto bytes = heartbeat_packet();

    for (auto _ : state) {
        const auto result = fgep::moldudp64::decode_downstream_packet(
            std::span<const std::byte>{bytes.data(), bytes.size()}
        );

        benchmark::DoNotOptimize(result);
    }
}

void BM_EncodedBackendSubmitEnter(benchmark::State& state) {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        state.PauseTiming();
        backend.clear();
        const auto message = enter_order(user_ref_num++);
        state.ResumeTiming();

        const auto result =
            fgep::execution::submit_enter_order_to_backend(backend, message);

        benchmark::DoNotOptimize(result);
    }
}

void BM_EncodedBackendSubmitReplace(benchmark::State& state) {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        state.PauseTiming();
        backend.clear();
        const auto original_ref = user_ref_num++;
        const auto replacement_ref = user_ref_num++;
        const auto message = replace_order(original_ref, replacement_ref);
        state.ResumeTiming();

        const auto result =
            fgep::execution::submit_replace_order_to_backend(backend, message);

        benchmark::DoNotOptimize(result);
    }
}

void BM_EncodedBackendSubmitCancel(benchmark::State& state) {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        state.PauseTiming();
        backend.clear();
        const auto message = cancel_order(user_ref_num++);
        state.ResumeTiming();

        const auto result =
            fgep::execution::submit_cancel_order_to_backend(backend, message);

        benchmark::DoNotOptimize(result);
    }
}

} // namespace

BENCHMARK(BM_OuchEncodeEnterOrder);
BENCHMARK(BM_OuchDecodeEnterOrder);
BENCHMARK(BM_MoldUdp64DecodeHeartbeat);
BENCHMARK(BM_EncodedBackendSubmitEnter);
BENCHMARK(BM_EncodedBackendSubmitReplace);
BENCHMARK(BM_EncodedBackendSubmitCancel);