if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fgep_benchmark_report.cpp")
    add_executable(fgep_benchmark_report fgep_benchmark_report.cpp)
    target_link_libraries(fgep_benchmark_report PRIVATE fgep)
endif()