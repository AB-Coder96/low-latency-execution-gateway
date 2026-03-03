#pragma once
#include "common/time_ns.h"
#include <cstdint>

namespace zl {

struct Message {
  uint64_t seq = 0;
  uint64_t t0_ns = 0;           // producer timestamp
  uint64_t t_parse_done_ns = 0; // optional per-stage
  uint64_t t_end_ns = 0;        // final stage timestamp
};

} // namespace zl
