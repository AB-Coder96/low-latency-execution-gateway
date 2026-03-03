#pragma once
#include <string>
#include <vector>

namespace zl {

// Returns lines like "key=value" or "# key=value" friendly content.
std::vector<std::string> collect_system_info();

} // namespace zl
