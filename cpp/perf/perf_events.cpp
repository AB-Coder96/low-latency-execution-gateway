#include "perf/perf_events.h"

#include <cstring>

#if ZL_OS_LINUX
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <sys/syscall.h>
  #include <linux/perf_event.h>
#endif

namespace zl {

#if ZL_OS_LINUX
static int perf_open(perf_event_attr* attr) {
  return static_cast<int>(syscall(SYS_perf_event_open, attr, 0 /*pid*/, -1 /*cpu*/, -1 /*group*/, 0));
}
static bool read_u64(int fd, uint64_t* out) {
  if (fd < 0) return false;
  uint64_t val = 0;
  if (read(fd, &val, sizeof(val)) != sizeof(val)) return false;
  *out = val;
  return true;
}
#endif

PerfEvents::~PerfEvents() {
#if ZL_OS_LINUX
  if (fd_cycles_ >= 0) close(fd_cycles_);
  if (fd_instructions_ >= 0) close(fd_instructions_);
#endif
}

bool PerfEvents::start(std::string* err) {
#if ZL_OS_LINUX
  perf_event_attr cycles{};
  cycles.type = PERF_TYPE_HARDWARE;
  cycles.size = sizeof(cycles);
  cycles.config = PERF_COUNT_HW_CPU_CYCLES;
  cycles.disabled = 1;
  cycles.exclude_kernel = 0;
  cycles.exclude_hv = 0;

  perf_event_attr instr{};
  instr.type = PERF_TYPE_HARDWARE;
  instr.size = sizeof(instr);
  instr.config = PERF_COUNT_HW_INSTRUCTIONS;
  instr.disabled = 1;
  instr.exclude_kernel = 0;
  instr.exclude_hv = 0;

  fd_cycles_ = perf_open(&cycles);
  if (fd_cycles_ < 0) {
    if (err) *err = "perf_event_open(cycles) failed (check perf_event_paranoid / permissions)";
    return false;
  }
  fd_instructions_ = perf_open(&instr);
  if (fd_instructions_ < 0) {
    if (err) *err = "perf_event_open(instructions) failed (check perf_event_paranoid / permissions)";
    return false;
  }

  ioctl(fd_cycles_, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd_instructions_, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd_cycles_, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fd_instructions_, PERF_EVENT_IOC_ENABLE, 0);
  return true;
#else
  if (err) *err = "perf counters only supported on Linux";
  return false;
#endif
}

PerfSnapshot PerfEvents::stop(std::string* err) {
  PerfSnapshot s{};
#if ZL_OS_LINUX
  if (fd_cycles_ < 0 || fd_instructions_ < 0) {
    if (err) *err = "perf not started";
    return s;
  }
  ioctl(fd_cycles_, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd_instructions_, PERF_EVENT_IOC_DISABLE, 0);
  if (!read_u64(fd_cycles_, &s.cycles) && err) *err = "read cycles failed";
  if (!read_u64(fd_instructions_, &s.instructions) && err) *err += " read instructions failed";
#else
  (void)err;
#endif
  return s;
}

} // namespace zl
