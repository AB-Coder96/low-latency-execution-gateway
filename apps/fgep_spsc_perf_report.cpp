#include "fgep/bench/system_metadata.hpp"

#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {

struct PerfEventCounts {
    std::uint64_t cycles{};
    std::uint64_t instructions{};
    std::uint64_t branches{};
    std::uint64_t branch_misses{};
    std::uint64_t cache_references{};
    std::uint64_t cache_misses{};
    std::uint64_t l1_dcache_loads{};
    std::uint64_t l1_dcache_load_misses{};
    std::uint64_t dtlb_loads{};
    std::uint64_t dtlb_load_misses{};
};

struct QueueRunStats {
    std::uint64_t produced{};
    std::uint64_t consumed{};
    std::uint64_t checksum{};
    std::uint64_t submit_queued{};
    std::uint64_t submit_rejected{};
    std::uint64_t ring_full{};
    std::size_t payload_size{};
};

struct PerfStatReport {
    std::string label{"unknown"};
    std::filesystem::path input_path{};
    std::string date_utc{};
    std::string host{};
    std::string kernel{};
    std::string command{};
    double elapsed_seconds{};
    PerfEventCounts events{};
    QueueRunStats queue{};
};

struct Args {
    std::filesystem::path input{};
    std::filesystem::path output{};
    std::string label{"local"};
};

[[nodiscard]] std::string trim(std::string_view text) {
    std::size_t first = 0U;

    while (
        first < text.size()
        && std::isspace(static_cast<unsigned char>(text[first])) != 0
    ) {
        ++first;
    }

    std::size_t last = text.size();

    while (
        last > first
        && std::isspace(static_cast<unsigned char>(text[last - 1U])) != 0
    ) {
        --last;
    }

    return std::string{text.substr(first, last - first)};
}

[[nodiscard]] std::optional<std::uint64_t> parse_u64_token(
    std::string_view text
) noexcept {
    std::string digits{};
    digits.reserve(text.size());

    for (const char value : text) {
        if (value >= '0' && value <= '9') {
            digits.push_back(value);
            continue;
        }

        if (value == ',') {
            continue;
        }

        if (!digits.empty()) {
            break;
        }
    }

    if (digits.empty()) {
        return std::nullopt;
    }

    std::uint64_t parsed{};
    const auto* first = digits.data();
    const auto* last = digits.data() + digits.size();

    const auto result = std::from_chars(first, last, parsed);

    if (result.ec != std::errc{} || result.ptr != last) {
        return std::nullopt;
    }

    return parsed;
}

[[nodiscard]] std::optional<double> parse_first_double(
    std::string_view text
) {
    std::string token{};

    for (const char value : text) {
        const bool numeric =
            (value >= '0' && value <= '9') || value == '.';

        if (numeric) {
            token.push_back(value);
            continue;
        }

        if (!token.empty()) {
            break;
        }
    }

    if (token.empty()) {
        return std::nullopt;
    }

    try {
        return std::stod(token);
    } catch (...) {
        return std::nullopt;
    }
}

[[nodiscard]] bool contains(
    std::string_view text,
    std::string_view needle
) noexcept {
    return text.find(needle) != std::string_view::npos;
}

void parse_event_line(
    const std::string& line,
    PerfEventCounts& events
) {
    const auto value = parse_u64_token(line);

    if (!value.has_value()) {
        return;
    }

    if (contains(line, "branch-misses")) {
        events.branch_misses = value.value();
    } else if (contains(line, "branches")) {
        events.branches = value.value();
    } else if (contains(line, "cache-references")) {
        events.cache_references = value.value();
    } else if (contains(line, "cache-misses")) {
        events.cache_misses = value.value();
    } else if (contains(line, "L1-dcache-load-misses")) {
        events.l1_dcache_load_misses = value.value();
    } else if (contains(line, "L1-dcache-loads")) {
        events.l1_dcache_loads = value.value();
    } else if (contains(line, "dTLB-load-misses")) {
        events.dtlb_load_misses = value.value();
    } else if (contains(line, "dTLB-loads")) {
        events.dtlb_loads = value.value();
    } else if (contains(line, "instructions")) {
        events.instructions = value.value();
    } else if (contains(line, "cycles")) {
        events.cycles = value.value();
    }
}

[[nodiscard]] std::optional<std::uint64_t> parse_key_value_u64(
    const std::string& line,
    std::string_view key
) {
    const std::string prefix = std::string{key} + "=";

    if (line.rfind(prefix, 0U) != 0U) {
        return std::nullopt;
    }

    return parse_u64_token(std::string_view{line}.substr(prefix.size()));
}

[[nodiscard]] std::optional<std::size_t> parse_key_value_size(
    const std::string& line,
    std::string_view key
) {
    const auto value = parse_key_value_u64(line, key);

    if (!value.has_value()) {
        return std::nullopt;
    }

    const auto converted = static_cast<std::size_t>(value.value());

    if (static_cast<std::uint64_t>(converted) != value.value()) {
        return std::nullopt;
    }

    return converted;
}

void parse_queue_line(
    const std::string& line,
    QueueRunStats& stats
) {
    if (const auto value = parse_key_value_size(line, "payload_size")) {
        stats.payload_size = value.value();
    } else if (const auto value = parse_key_value_u64(line, "produced")) {
        stats.produced = value.value();
    } else if (const auto value = parse_key_value_u64(line, "consumed")) {
        stats.consumed = value.value();
    } else if (const auto value = parse_key_value_u64(line, "checksum")) {
        stats.checksum = value.value();
    } else if (const auto value = parse_key_value_u64(line, "submit_queued")) {
        stats.submit_queued = value.value();
    } else if (
        const auto value = parse_key_value_u64(line, "submit_rejected")
    ) {
        stats.submit_rejected = value.value();
    } else if (const auto value = parse_key_value_u64(line, "ring_full")) {
        stats.ring_full = value.value();
    }
}

[[nodiscard]] std::string parse_header_value(
    const std::string& line,
    std::string_view key
) {
    const std::string prefix = "# " + std::string{key} + ": ";

    if (line.rfind(prefix, 0U) != 0U) {
        return {};
    }

    return trim(std::string_view{line}.substr(prefix.size()));
}

[[nodiscard]] PerfStatReport parse_perf_file(
    const std::filesystem::path& input,
    std::string label
) {
    std::ifstream stream{input};

    if (!stream) {
        throw std::runtime_error{"failed to open perf input"};
    }

    PerfStatReport report{};
    report.label = std::move(label);
    report.input_path = input;

    std::string line{};

    while (std::getline(stream, line)) {
        if (line.rfind("# ", 0U) == 0U) {
            if (auto value = parse_header_value(line, "date_utc");
                !value.empty()) {
                report.date_utc = std::move(value);
            } else if (auto value = parse_header_value(line, "host");
                       !value.empty()) {
                report.host = std::move(value);
            } else if (auto value = parse_header_value(line, "kernel");
                       !value.empty()) {
                report.kernel = std::move(value);
            } else if (auto value = parse_header_value(line, "command");
                       !value.empty()) {
                report.command = std::move(value);
            }

            continue;
        }

        if (contains(line, "seconds time elapsed")) {
            if (const auto value = parse_first_double(line)) {
                report.elapsed_seconds = value.value();
            }

            continue;
        }

        parse_event_line(line, report.events);
        parse_queue_line(line, report.queue);
    }

    return report;
}

[[nodiscard]] double safe_divide(
    double numerator,
    double denominator
) noexcept {
    if (denominator == 0.0) {
        return 0.0;
    }

    return numerator / denominator;
}

[[nodiscard]] double percentage(
    std::uint64_t numerator,
    std::uint64_t denominator
) noexcept {
    return 100.0 * safe_divide(
        static_cast<double>(numerator),
        static_cast<double>(denominator)
    );
}

[[nodiscard]] std::string fixed(
    double value,
    int precision = 3
) {
    std::ostringstream output{};
    output << std::fixed << std::setprecision(precision) << value;
    return output.str();
}

[[nodiscard]] std::string timestamp_from_date_utc(
    std::string_view date_utc
) {
    // Input:  2026-05-19T04:19:48Z
    // Output: 20260519-041948
    if (date_utc.size() >= 19U) {
        std::string timestamp{};

        timestamp.reserve(15U);
        timestamp.append(date_utc.substr(0U, 4U));
        timestamp.append(date_utc.substr(5U, 2U));
        timestamp.append(date_utc.substr(8U, 2U));
        timestamp.push_back('-');
        timestamp.append(date_utc.substr(11U, 2U));
        timestamp.append(date_utc.substr(14U, 2U));
        timestamp.append(date_utc.substr(17U, 2U));

        return timestamp;
    }

    return "unknown-time";
}

[[nodiscard]] std::string sanitize_filename_token(
    std::string_view value
) {
    std::string output{};

    output.reserve(value.size());

    for (const char character : value) {
        const auto byte = static_cast<unsigned char>(character);

        if (
            std::isalnum(byte) != 0
            || character == '-'
            || character == '_'
            || character == '.'
        ) {
            output.push_back(character);
        } else {
            output.push_back('-');
        }
    }

    if (output.empty()) {
        return "local";
    }

    return output;
}

[[nodiscard]] std::filesystem::path default_report_output_path(
    const PerfStatReport& report
) {
    const auto label = sanitize_filename_token(report.label);
    const auto timestamp = timestamp_from_date_utc(report.date_utc);

    return std::filesystem::path{"reports"}
        / "perf"
        / (label + "-spsc-queue-report-" + timestamp + ".md");
}

void append_event_row(
    std::ostringstream& output,
    std::string_view name,
    std::uint64_t value
) {
    output << "| " << name << " | " << value << " |\n";
}

[[nodiscard]] std::string format_report(
    const PerfStatReport& report
) {
    const auto& events = report.events;
    const auto& queue = report.queue;

    const auto handoffs_per_second = safe_divide(
        static_cast<double>(queue.consumed),
        report.elapsed_seconds
    );

    const auto mhandoffs_per_second = handoffs_per_second / 1'000'000.0;

    const auto ipc = safe_divide(
        static_cast<double>(events.instructions),
        static_cast<double>(events.cycles)
    );

    const auto cycles_per_handoff = safe_divide(
        static_cast<double>(events.cycles),
        static_cast<double>(queue.consumed)
    );

    const auto instructions_per_handoff = safe_divide(
        static_cast<double>(events.instructions),
        static_cast<double>(queue.consumed)
    );

    std::ostringstream output{};

    output << "# SPSC Queue Perf Report\n\n";
    output << "Label: `" << report.label << "`\n\n";

    output << "## Input\n\n";
    output << "| Field | Value |\n";
    output << "|---|---|\n";
    output << "| input_path | `" << report.input_path.string() << "` |\n";
    output << "| date_utc | " << report.date_utc << " |\n";
    output << "| host | " << report.host << " |\n";
    output << "| kernel | " << report.kernel << " |\n";
    output << "| command | `" << report.command << "` |\n";

    output << "\n## Queue run summary\n\n";
    output << "| Metric | Value |\n";
    output << "|---|---:|\n";
    output << "| payload_size_bytes | " << queue.payload_size << " |\n";
    output << "| produced | " << queue.produced << " |\n";
    output << "| consumed | " << queue.consumed << " |\n";
    output << "| submit_queued | " << queue.submit_queued << " |\n";
    output << "| submit_rejected | " << queue.submit_rejected << " |\n";
    output << "| ring_full | " << queue.ring_full << " |\n";
    output << "| elapsed_seconds | " << fixed(report.elapsed_seconds, 6) << " |\n";
    output << "| throughput_handoffs_per_sec | "
           << fixed(handoffs_per_second, 2) << " |\n";
    output << "| throughput_mhandoffs_per_sec | "
           << fixed(mhandoffs_per_second, 3) << " |\n";

    output << "\n## Perf counters\n\n";
    output << "| Counter | Value |\n";
    output << "|---|---:|\n";
    append_event_row(output, "cycles", events.cycles);
    append_event_row(output, "instructions", events.instructions);
    append_event_row(output, "branches", events.branches);
    append_event_row(output, "branch_misses", events.branch_misses);
    append_event_row(output, "cache_references", events.cache_references);
    append_event_row(output, "cache_misses", events.cache_misses);
    append_event_row(output, "l1_dcache_loads", events.l1_dcache_loads);
    append_event_row(
        output,
        "l1_dcache_load_misses",
        events.l1_dcache_load_misses
    );
    append_event_row(output, "dtlb_loads", events.dtlb_loads);
    append_event_row(output, "dtlb_load_misses", events.dtlb_load_misses);

    output << "\n## Derived metrics\n\n";
    output << "| Metric | Value |\n";
    output << "|---|---:|\n";
    output << "| ipc_instructions_per_cycle | " << fixed(ipc, 3) << " |\n";
    output << "| cycles_per_handoff | " << fixed(cycles_per_handoff, 2)
           << " |\n";
    output << "| instructions_per_handoff | "
           << fixed(instructions_per_handoff, 2) << " |\n";
    output << "| branch_miss_rate_percent | "
           << fixed(percentage(events.branch_misses, events.branches), 4)
           << " |\n";
    output << "| l1d_miss_rate_percent | "
           << fixed(
                  percentage(
                      events.l1_dcache_load_misses,
                      events.l1_dcache_loads
                  ),
                  4
              )
           << " |\n";
    output << "| dtlb_miss_rate_percent | "
           << fixed(percentage(events.dtlb_load_misses, events.dtlb_loads), 6)
           << " |\n";
    output << "| ring_full_rate_percent | "
           << fixed(
                  percentage(
                      queue.ring_full,
                      queue.submit_queued + queue.submit_rejected
                  ),
                  4
              )
           << " |\n";

    output << "\n";
    output << fgep::bench::format_system_metadata_markdown(
        fgep::bench::collect_system_metadata()
    );

    return output.str();
}

[[nodiscard]] bool parse_args(
    int argc,
    char** argv,
    Args& args
) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};

        if (arg == "--input") {
            if (index + 1 >= argc) {
                return false;
            }

            ++index;
            args.input = argv[index];
            continue;
        }

        if (arg == "--output") {
            if (index + 1 >= argc) {
                return false;
            }

            ++index;
            args.output = argv[index];
            continue;
        }

        if (arg == "--label") {
            if (index + 1 >= argc) {
                return false;
            }

            ++index;
            args.label = argv[index];
            continue;
        }

        return false;
    }

    return !args.input.empty();
}

void print_usage(const char* program) {
    std::cerr
        << "usage: " << program
        << " --input reports/perf/ec2-spsc-queue-perf.txt"
        << " [--output reports/perf/ec2-spsc-queue-report.md]"
        << " --label ec2\n";
}

} // namespace

int main(int argc, char** argv) {
    Args args{};

    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 2;
    }

    try {
        const auto report = parse_perf_file(args.input, args.label);
        const auto markdown = format_report(report);

        if (args.output.empty()) {
            args.output = default_report_output_path(report);
        }

        const auto parent_path = args.output.parent_path();

        if (!parent_path.empty()) {
            std::filesystem::create_directories(parent_path);
        }

        std::ofstream output{args.output};

        if (!output) {
            std::cerr << "failed to open output path: "
                      << args.output << '\n';
            return 1;
        }

        output << markdown;

        std::cout << "wrote SPSC perf report to: "
                  << args.output.string() << '\n';

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "failed to generate SPSC perf report: "
                  << error.what() << '\n';
        return 1;
    }
}