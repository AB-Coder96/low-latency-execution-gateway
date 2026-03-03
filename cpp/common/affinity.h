#pragma once
#include "platform.h"
#include <string>

namespace zl {

struct AffinityResult {
  bool ok;
  std::string msg;
};

AffinityResult pin_current_thread_to_cpu(int cpu);

} // namespace zl
