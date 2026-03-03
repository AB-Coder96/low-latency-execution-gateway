#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace zl {

struct Percentiles {
  double p50 = 0, p95 = 0, p99 = 0, p999 = 0;
};

inline Percentiles compute_percentiles(std::vector<uint64_t> v) {
  Percentiles p{};
  if (v.empty()) return p;
  std::sort(v.begin(), v.end());
  auto at = [&](double q)->double {
    if (v.empty()) return 0.0;
    double idx = q * (v.size() - 1);
    auto i = static_cast<size_t>(idx);
    auto j = std::min(i + 1, v.size() - 1);
    double frac = idx - i;
    return (1.0 - frac) * static_cast<double>(v[i]) + frac * static_cast<double>(v[j]);
  };
  p.p50 = at(0.50);
  p.p95 = at(0.95);
  p.p99 = at(0.99);
  p.p999 = at(0.999);
  return p;
}

} // namespace zl
