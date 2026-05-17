#include "fgep/bench/system_metadata.hpp"

#include <sys/utsname.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>

#ifndef FGEP_BUILD_TYPE
#define FGEP_BUILD_TYPE "unknown"
#endif

#ifndef FGEP_DEFAULT_EXEC_BACKEND
#define FGEP_DEFAULT_EXEC_BACKEND "kernel_udp"
#endif

#ifndef FGEP_GIT_COMMIT
#define FGEP_GIT_COMMIT "unknown"
#endif

#ifndef FGEP_HAVE_DPDK
#define FGEP_HAVE_DPDK 0
#endif

#ifndef FGEP_HAVE_AFXDP
#define FGEP_HAVE_AFXDP 0
#endif

namespace fgep::bench {
namespace {

[[nodiscard]] std::string trim(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }

    return text;
}

[[nodiscard]] std::string local_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

#if defined(_WIN32)
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);

    return buffer;
}

[[nodiscard]] std::string hostname() {
    char buffer[256]{};

    if (::gethostname(buffer, sizeof(buffer)) != 0) {
        return "unknown";
    }

    buffer[sizeof(buffer) - 1U] = '\0';
    return buffer;
}

[[nodiscard]] std::string os_pretty_name() {
    std::ifstream input{"/etc/os-release"};

    if (!input) {
        return "unknown";
    }

    std::string line{};

    while (std::getline(input, line)) {
        constexpr std::string_view key{"PRETTY_NAME="};

        if (line.rfind(key, 0) == 0) {
            auto value = line.substr(key.size());

            if (value.size() >= 2U && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2U);
            }

            return value;
        }
    }

    return "unknown";
}

[[nodiscard]] std::string cpu_model() {
    std::ifstream input{"/proc/cpuinfo"};

    if (!input) {
        return "unknown";
    }

    std::string line{};

    while (std::getline(input, line)) {
        constexpr std::string_view key{"model name"};

        if (line.rfind(key, 0) == 0) {
            const auto separator = line.find(':');

            if (separator != std::string::npos) {
                return trim(line.substr(separator + 2U));
            }
        }
    }

    return "unknown";
}

[[nodiscard]] std::string compiler_string() {
#if defined(__clang__)
    return std::string{"clang "} + __clang_version__;
#elif defined(__GNUC__)
    return std::string{"gcc "}
        + std::to_string(__GNUC__)
        + "."
        + std::to_string(__GNUC_MINOR__)
        + "."
        + std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    return std::string{"msvc "} + std::to_string(_MSC_VER);
#else
    return "unknown";
#endif
}

} // namespace

SystemMetadata collect_system_metadata() {
    utsname uname_info{};

    std::string kernel_name{"unknown"};
    std::string kernel_release{"unknown"};
    std::string kernel_version{"unknown"};
    std::string machine{"unknown"};

    if (::uname(&uname_info) == 0) {
        kernel_name = uname_info.sysname;
        kernel_release = uname_info.release;
        kernel_version = uname_info.version;
        machine = uname_info.machine;
    }

    return SystemMetadata{
        .generated_at_local = local_timestamp(),
        .hostname = hostname(),
        .os_name = os_pretty_name(),
        .kernel_name = kernel_name,
        .kernel_release = kernel_release,
        .kernel_version = kernel_version,
        .machine = machine,
        .cpu_model = cpu_model(),
        .compiler = compiler_string(),
        .build_type = FGEP_BUILD_TYPE,
        .default_backend = FGEP_DEFAULT_EXEC_BACKEND,
        .git_commit = FGEP_GIT_COMMIT,
        .have_dpdk = FGEP_HAVE_DPDK != 0,
        .have_afxdp = FGEP_HAVE_AFXDP != 0
    };
}

std::string format_system_metadata_markdown(
    const SystemMetadata& metadata
) {
    std::ostringstream output{};

    output << "## System metadata\n\n";
    output << "| Field | Value |\n";
    output << "|---|---|\n";
    output << "| generated_at_local | " << metadata.generated_at_local << " |\n";
    output << "| hostname | " << metadata.hostname << " |\n";
    output << "| os_name | " << metadata.os_name << " |\n";
    output << "| kernel_name | " << metadata.kernel_name << " |\n";
    output << "| kernel_release | " << metadata.kernel_release << " |\n";
    output << "| kernel_version | " << metadata.kernel_version << " |\n";
    output << "| machine | " << metadata.machine << " |\n";
    output << "| cpu_model | " << metadata.cpu_model << " |\n";
    output << "| compiler | " << metadata.compiler << " |\n";
    output << "| build_type | " << metadata.build_type << " |\n";
    output << "| default_backend | " << metadata.default_backend << " |\n";
    output << "| git_commit | " << metadata.git_commit << " |\n";
    output << "| have_dpdk | " << bool_text(metadata.have_dpdk) << " |\n";
    output << "| have_afxdp | " << bool_text(metadata.have_afxdp) << " |\n";

    return output.str();
}

const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

} // namespace fgep::bench