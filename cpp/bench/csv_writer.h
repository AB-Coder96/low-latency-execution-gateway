#pragma once
#include <fstream>
#include <string>
#include <vector>

namespace zl {

class CsvWriter {
 public:
  explicit CsvWriter(std::string path) : path_(std::move(path)), out_(path_) {}

  bool ok() const { return out_.good(); }

  void write_metadata(const std::vector<std::string>& kv_lines) {
    for (const auto& kv : kv_lines) out_ << "# " << kv << "\n";
  }

  void write_header(const std::string& header) {
    out_ << header << "\n";
  }

  template <typename... Ts>
  void write_row(Ts... cols) {
    write_cols(cols...);
    out_ << "\n";
  }

 private:
  template <typename T>
  void write_one(const T& v) { out_ << v; }

  void write_cols() {}

  template <typename T, typename... Ts>
  void write_cols(const T& a, Ts... rest) {
    write_one(a);
    if constexpr (sizeof...(rest) > 0) out_ << ",";
    write_cols(rest...);
  }

  std::string path_;
  std::ofstream out_;
};

} // namespace zl
