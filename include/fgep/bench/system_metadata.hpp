#pragma once

#include <string>

namespace fgep::bench {

struct SystemMetadata {
    std::string generated_at_local{};
    std::string hostname{};
    std::string os_name{};
    std::string kernel_name{};
    std::string kernel_release{};
    std::string kernel_version{};
    std::string machine{};
    std::string cpu_model{};
    std::string compiler{};
    std::string build_type{};
    std::string default_backend{};
    std::string git_commit{};
    bool have_dpdk{};
    bool have_afxdp{};
};

[[nodiscard]] SystemMetadata collect_system_metadata();

[[nodiscard]] std::string format_system_metadata_markdown(
    const SystemMetadata& metadata
);

[[nodiscard]] const char* bool_text(bool value) noexcept;

} // namespace fgep::bench