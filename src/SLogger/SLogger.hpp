#pragma once
#include <string_view>
#include <fstream>

class SLogger {
public:
  SLogger() = default;
  SLogger(std::string_view file_name);
  SLogger(std::wstring_view file_name);
  void log(std::string to_write);
private:
  std::string_view fname;
  std::ofstream log_stm;
};
