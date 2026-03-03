#include "bench/scenarios.h"
#include "common/cli.h"

#include <filesystem>
#include <iostream>

using namespace zl;

static void usage() {
  std::cout <<
R"(ZetaLatency_bench

Required:
  --scenario  spsc | mpsc | pipeline
  --queue     ring | mutex | mpsc | (pipeline: q1,q2 e.g. ring,ring or ring,mutex)
  --out       path/to/output.csv

Optional:
  --seconds N          measurement seconds (default 10)
  --warmup  N          warmup seconds (default 3)
  --ring-capacity N    ring buffer capacity (power-of-two, default 65536)
  --producers N        MPSC producers (default 4)

Pinning (Linux only):
  --pin-producer CPU
  --pin-consumer CPU
  --pin-parse    CPU     (pipeline)
  --pin-book     CPU     (pipeline)

Pipeline work:
  --parse-work ITERS    (default 200)
  --book-work  ITERS    (default 200)
  --per-stage           write per-stage columns for pipeline CSV

Linux-only perf counters (best-effort; may require permissions):
  --perf                collect cycles + instructions over the run

Examples:
  ./ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --out out/spsc_ring.csv
  ./ZetaLatency_bench --scenario mpsc --queue mpsc --producers 6 --seconds 10 --warmup 3 --out out/mpsc.csv
  ./ZetaLatency_bench --scenario pipeline --queue ring,ring --seconds 10 --warmup 3 --per-stage --out out/pipeline.csv
)";
}

int main(int argc, char** argv) {
  Cli cli(argc, argv);
  if (cli.has("help") || cli.has("h")) {
    usage();
    return 0;
  }

  RunConfig cfg;
  cfg.scenario = cli.get("scenario", "spsc");
  cfg.queue = cli.get("queue", "ring");
  cfg.seconds = cli.get_int("seconds").value_or(10);
  cfg.warmup = cli.get_int("warmup").value_or(3);
  cfg.ring_capacity = cli.get_int("ring-capacity").value_or(1 << 16);
  cfg.producers = cli.get_int("producers").value_or(4);
  cfg.pin_producer = cli.get_int("pin-producer").value_or(-1);
  cfg.pin_consumer = cli.get_int("pin-consumer").value_or(-1);
  cfg.pin_parse = cli.get_int("pin-parse").value_or(-1);
  cfg.pin_book = cli.get_int("pin-book").value_or(-1);
  cfg.per_stage = cli.has("per-stage");
  cfg.perf = cli.has("perf");
  cfg.parse_work = static_cast<uint32_t>(cli.get_int("parse-work").value_or(200));
  cfg.book_work = static_cast<uint32_t>(cli.get_int("book-work").value_or(200));

  auto out_path = cli.get("out", "");
  if (out_path.empty()) {
    std::cerr << "ERROR: --out is required\n\n";
    usage();
    return 2;
  }
  auto parent = std::filesystem::path(out_path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);

  CsvWriter csv(out_path);
  if (!csv.ok()) {
    std::cerr << "ERROR: cannot open output file: " << out_path << "\n";
    return 2;
  }

  // metadata
  std::vector<std::string> meta = collect_system_info();
  meta.push_back("scenario=" + cfg.scenario);
  meta.push_back("queue=" + cfg.queue);
  meta.push_back("seconds=" + std::to_string(cfg.seconds));
  meta.push_back("warmup=" + std::to_string(cfg.warmup));
  meta.push_back("ring_capacity=" + std::to_string(cfg.ring_capacity));
  meta.push_back("producers=" + std::to_string(cfg.producers));
  meta.push_back("pin_producer=" + std::to_string(cfg.pin_producer));
  meta.push_back("pin_consumer=" + std::to_string(cfg.pin_consumer));
  meta.push_back("pin_parse=" + std::to_string(cfg.pin_parse));
  meta.push_back("pin_book=" + std::to_string(cfg.pin_book));
  meta.push_back("per_stage=" + std::string(cfg.per_stage ? "1" : "0"));
  meta.push_back("perf=" + std::string(cfg.perf ? "1" : "0"));
  csv.write_metadata(meta);

  if (cfg.scenario == "spsc") {
    if (cfg.queue == "ring") {
      SpscRingBuffer<Message> q(static_cast<size_t>(cfg.ring_capacity));
      run_spsc(q, cfg, csv);
      return 0;
    }
    if (cfg.queue == "mutex") {
      MutexQueue<Message> q;
      run_spsc_mutex(q, cfg, csv);
      return 0;
    }
    std::cerr << "ERROR: spsc queue must be ring|mutex\n";
    return 2;
  }

  if (cfg.scenario == "mpsc") {
    // queue must be mpsc
    run_mpsc(cfg, csv);
    return 0;
  }

  if (cfg.scenario == "pipeline") {
    // queue format: q1,q2
    auto pos = cfg.queue.find(',');
    std::string q1s = (pos == std::string::npos) ? cfg.queue : cfg.queue.substr(0, pos);
    std::string q2s = (pos == std::string::npos) ? cfg.queue : cfg.queue.substr(pos + 1);

    auto make_ring = [&]{ return SpscRingBuffer<Message>(static_cast<size_t>(cfg.ring_capacity)); };

    // For a staged pipeline we use SPSC between stages.
    if (q1s == "ring" && q2s == "ring") {
      auto q1 = make_ring();
      auto q2 = make_ring();
      run_pipeline(q1, q2, cfg, csv);
      return 0;
    }
    if (q1s == "ring" && q2s == "mutex") {
      auto q1 = make_ring();
      MutexQueue<Message> q2;
      run_pipeline(q1, q2, cfg, csv);
      return 0;
    }
    if (q1s == "mutex" && q2s == "ring") {
      MutexQueue<Message> q1;
      auto q2 = make_ring();
      run_pipeline(q1, q2, cfg, csv);
      return 0;
    }
    if (q1s == "mutex" && q2s == "mutex") {
      MutexQueue<Message> q1;
      MutexQueue<Message> q2;
      run_pipeline(q1, q2, cfg, csv);
      return 0;
    }

    std::cerr << "ERROR: pipeline queue must be one of ring|mutex (e.g. ring,ring)\n";
    return 2;
  }

  std::cerr << "ERROR: unknown scenario: " << cfg.scenario << "\n\n";
  usage();
  return 2;
}
