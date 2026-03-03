#pragma once
#include "common/platform.h"
#include <cstdint>
#include <string>

namespace zl {

struct PerfSnapshot {
  uint64_t cycles = 0;
  uint64_t instructions = 0;
};

class PerfEvents {
 public:
  PerfEvents() = default;
  ~PerfEvents();

  // Returns false if not supported or failed.
  bool start(std::string* err = nullptr);
  PerfSnapshot stop(std::string* err = nullptr);

 private:
#if ZL_OS_LINUX
  int fd_cycles_ = -1;
  int fd_instructions_ = -1;
#endif
};

} // namespace zl
