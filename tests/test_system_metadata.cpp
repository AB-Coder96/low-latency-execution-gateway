#include "fgep/bench/system_metadata.hpp"

#include <cassert>
#include <string_view>

namespace {

[[nodiscard]] bool contains(
    std::string_view text,
    std::string_view needle
) noexcept {
    return text.find(needle) != std::string_view::npos;
}

} // namespace

int main() {
    using namespace fgep::bench;

    {
        assert(std::string_view{bool_text(true)} == "true");
        assert(std::string_view{bool_text(false)} == "false");
    }

    {
        const auto metadata = collect_system_metadata();

        assert(!metadata.generated_at_local.empty());
        assert(!metadata.hostname.empty());
        assert(!metadata.os_name.empty());
        assert(!metadata.kernel_name.empty());
        assert(!metadata.kernel_release.empty());
        assert(!metadata.kernel_version.empty());
        assert(!metadata.machine.empty());
        assert(!metadata.cpu_model.empty());
        assert(!metadata.compiler.empty());
        assert(!metadata.build_type.empty());
        assert(!metadata.default_backend.empty());
        assert(!metadata.git_commit.empty());

        const auto markdown = format_system_metadata_markdown(metadata);

        assert(contains(markdown, "## System metadata"));
        assert(contains(markdown, "| generated_at_local |"));
        assert(contains(markdown, "| hostname |"));
        assert(contains(markdown, "| os_name |"));
        assert(contains(markdown, "| kernel_name |"));
        assert(contains(markdown, "| kernel_release |"));
        assert(contains(markdown, "| machine |"));
        assert(contains(markdown, "| cpu_model |"));
        assert(contains(markdown, "| compiler |"));
        assert(contains(markdown, "| build_type |"));
        assert(contains(markdown, "| default_backend |"));
        assert(contains(markdown, "| git_commit |"));
        assert(contains(markdown, "| have_dpdk |"));
        assert(contains(markdown, "| have_afxdp |"));
    }

    {
        const SystemMetadata metadata{
            .generated_at_local = "2026-05-17 12:00:00",
            .hostname = "test-host",
            .os_name = "Test Linux",
            .kernel_name = "Linux",
            .kernel_release = "6.1.0",
            .kernel_version = "test-version",
            .machine = "x86_64",
            .cpu_model = "Test CPU",
            .compiler = "gcc test",
            .build_type = "Debug",
            .default_backend = "kernel_udp",
            .git_commit = "abc123",
            .have_dpdk = true,
            .have_afxdp = false
        };

        const auto markdown = format_system_metadata_markdown(metadata);

        assert(contains(markdown, "| hostname | test-host |"));
        assert(contains(markdown, "| os_name | Test Linux |"));
        assert(contains(markdown, "| cpu_model | Test CPU |"));
        assert(contains(markdown, "| default_backend | kernel_udp |"));
        assert(contains(markdown, "| git_commit | abc123 |"));
        assert(contains(markdown, "| have_dpdk | true |"));
        assert(contains(markdown, "| have_afxdp | false |"));
    }

    return 0;
}