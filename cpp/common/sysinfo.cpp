#include "common/sysinfo.h"
#include "common/platform.h"
#include <fstream>
#include <sstream>

#if ZL_OS_LINUX
  #include <sys/utsname.h>
#endif

namespace zl {

static std::string read_first_line(const std::string& path) {
  std::ifstream f(path);
  std::string line;
  if (f.good() && std::getline(f, line)) return line;
  return "";
}

static std::string cpu_model_linux() {
#if ZL_OS_LINUX
  std::ifstream f("/proc/cpuinfo");
  std::string line;
  while (std::getline(f, line)) {
    if (line.rfind("model name", 0) == 0) {
      auto pos = line.find(':');
      if (pos != std::string::npos) return line.substr(pos + 2);
    }
  }
#endif
  return "";
}

std::vector<std::string> collect_system_info() {
  std::vector<std::string> out;
  out.push_back(std::string("git_sha=") + ZL_GIT_SHA);

#if ZL_OS_LINUX
  utsname u{};
  if (uname(&u) == 0) {
    out.push_back(std::string("kernel_sysname=") + u.sysname);
    out.push_back(std::string("kernel_release=") + u.release);
    out.push_back(std::string("kernel_version=") + u.version);
    out.push_back(std::string("machine=") + u.machine);
  }
  auto model = cpu_model_linux();
  if (!model.empty()) out.push_back("cpu_model=" + model);

  // Governor (best-effort; may not exist in containers)
  auto gov = read_first_line("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
  if (!gov.empty()) out.push_back("cpu_governor=" + gov);
#endif
  return out;
}

} // namespace zl
