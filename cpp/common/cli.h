#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>

namespace zl {

class Cli {
 public:
  explicit Cli(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
      std::string s = argv[i];
      if (s.rfind("--", 0) == 0) {
        std::string key = s.substr(2);
        std::string val = "1";
        if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
          val = argv[++i];
        }
        opts_[key] = val;
      } else {
        positionals_.push_back(s);
      }
    }
  }

  bool has(std::string_view key) const {
    return opts_.find(std::string(key)) != opts_.end();
  }

  std::string get(std::string_view key, std::string def="") const {
    auto it = opts_.find(std::string(key));
    return it == opts_.end() ? def : it->second;
  }

  std::optional<int> get_int(std::string_view key) const {
    auto it = opts_.find(std::string(key));
    if (it == opts_.end()) return std::nullopt;
    try { return std::stoi(it->second); } catch(...) { return std::nullopt; }
  }

  std::optional<long long> get_ll(std::string_view key) const {
    auto it = opts_.find(std::string(key));
    if (it == opts_.end()) return std::nullopt;
    try { return std::stoll(it->second); } catch(...) { return std::nullopt; }
  }

  std::vector<std::string> positionals() const { return positionals_; }

 private:
  std::unordered_map<std::string,std::string> opts_;
  std::vector<std::string> positionals_;
};

} // namespace zl
