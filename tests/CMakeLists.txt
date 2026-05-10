# Test build configuration for FPGA-Gated Execution Platform.
#
# This file defines a small helper function for adding C++ test executables.
# Individual tests are added only when their source files exist, so the repo
# can configure cleanly during the early skeleton stage.

function(fgep_add_test test_name source_file)
    add_executable(
        ${test_name}
        ${source_file}
    )

    target_link_libraries(
        ${test_name}
        PRIVATE
            fgep
    )

    target_compile_features(
        ${test_name}
        PRIVATE
            cxx_std_20
    )

    add_test(
        NAME ${test_name}
        COMMAND ${test_name}
    )
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_core_types.cpp")
    fgep_add_test(test_core_types test_core_types.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_errors.cpp")
    fgep_add_test(test_errors test_errors.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wire_byte_io.cpp")
    fgep_add_test(test_wire_byte_io test_wire_byte_io.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_fixed_ascii.cpp")
    fgep_add_test(test_fixed_ascii test_fixed_ascii.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_itch_message_type.cpp")
    fgep_add_test(test_itch_message_type test_itch_message_type.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_itch_decode_encode.cpp")
    fgep_add_test(test_itch_decode_encode test_itch_decode_encode.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ouch_decode_encode.cpp")
    fgep_add_test(test_ouch_decode_encode test_ouch_decode_encode.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_market_events.cpp")
    fgep_add_test(test_market_events test_market_events.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_order_events.cpp")
    fgep_add_test(test_order_events test_order_events.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_moldudp64_decode_encode.cpp")
    fgep_add_test(test_moldudp64_decode_encode test_moldudp64_decode_encode.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_moldudp64_itch_replay.cpp")
    fgep_add_test(
        test_moldudp64_itch_replay
        test_moldudp64_itch_replay.cpp
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_moldudp64_itch_integration.cpp")
    fgep_add_test(
        test_moldudp64_itch_integration
        test_moldudp64_itch_integration.cpp
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_memory_replay_source.cpp")
    fgep_add_test(test_memory_replay_source test_memory_replay_source.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_csv_replay_source.cpp")
    fgep_add_test(test_csv_replay_source test_csv_replay_source.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_order_book.cpp")
    fgep_add_test(test_order_book test_order_book.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_normalized_bbo.cpp")
    fgep_add_test(test_normalized_bbo test_normalized_bbo.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lifecycle_engine.cpp")
    fgep_add_test(test_lifecycle_engine test_lifecycle_engine.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_risk_supervisor.cpp")
    fgep_add_test(test_risk_supervisor test_risk_supervisor.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_permissive_state.cpp")
    fgep_add_test(test_permissive_state test_permissive_state.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cpu_pipeline.cpp")
    fgep_add_test(test_cpu_pipeline test_cpu_pipeline.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_telemetry_recorder.cpp")
    fgep_add_test(test_telemetry_recorder test_telemetry_recorder.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_latency_stats.cpp")
    fgep_add_test(test_latency_stats test_latency_stats.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mock_execution_backend.cpp")
    fgep_add_test(test_mock_execution_backend test_mock_execution_backend.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_kernel_udp_execution_backend.cpp")
    fgep_add_test(test_kernel_udp_execution_backend test_kernel_udp_execution_backend.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_simulated_hardware_gate.cpp")
    fgep_add_test(test_simulated_hardware_gate test_simulated_hardware_gate.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_zero_post_halt_leak.cpp")
    fgep_add_test(test_zero_post_halt_leak test_zero_post_halt_leak.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_itch_stock_directory_decode_encode.cpp")
    fgep_add_test(
        test_itch_stock_directory_decode_encode
        test_itch_stock_directory_decode_encode.cpp
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_itch_stock_trading_action_decode_encode.cpp")
    fgep_add_test(
        test_itch_stock_trading_action_decode_encode
        test_itch_stock_trading_action_decode_encode.cpp
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_itch_instrument_directory_integration.cpp")
    fgep_add_test(
        test_itch_instrument_directory_integration
        test_itch_instrument_directory_integration.cpp
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_multi_venue_book_and_bbo.cpp")
    fgep_add_test(
        test_multi_venue_book_and_bbo
        test_multi_venue_book_and_bbo.cpp
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_instrument_directory.cpp")
    fgep_add_test(test_instrument_directory test_instrument_directory.cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_replay_to_books_integration.cpp")
    fgep_add_test(
        test_replay_to_books_integration
        test_replay_to_books_integration.cpp
    )
endif()