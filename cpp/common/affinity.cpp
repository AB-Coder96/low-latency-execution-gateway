#include "common/affinity.h"

#if ZL_OS_LINUX
  #include <pthread.h>
  #include <sched.h>
  #include <errno.h>
#endif

namespace zl {

AffinityResult pin_current_thread_to_cpu(int cpu) {
#if ZL_OS_LINUX
  if (cpu < 0) return {true, "pin disabled"};
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(static_cast<unsigned>(cpu), &set);
  int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
  if (rc != 0) {
    return {false, "pthread_setaffinity_np failed (errno=" + std::to_string(rc) + ")"};
  }
  return {true, "pinned to cpu " + std::to_string(cpu)};
#else
  (void)cpu;
  return {false, "thread pinning only supported on Linux"};
#endif
}

} // namespace zl
