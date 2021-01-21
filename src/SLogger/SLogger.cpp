#include "SLogger.hpp"
#include <iostream>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;


SLogger::SLogger(std::string_view file_name) {
  size_t dir_name_size = file_name.find_last_of('\\');
  fs::create_directories(file_name.substr(0, dir_name_size));  
  log_stm.open(file_name.data(), std::ifstream::out);
}

SLogger::SLogger(std::wstring_view file_name) {
  size_t dir_name_size = file_name.find_last_of(L'\\');
  fs::create_directories(file_name.substr(0, dir_name_size));  
  log_stm.open(file_name.data(), std::ifstream::out);
}

void SLogger::log(std::string to_write) {
  char prefix_buf[23] = {};

  const std::time_t now = std::time(nullptr);
  tm t;
  localtime_s(&t, &now);

  std::strftime(prefix_buf, sizeof(prefix_buf), "[%d/%m/%Y-%H:%M:%S] ", &t);
  std::string log_line = prefix_buf;
  log_line += to_write + "\n";
  log_stm << log_line;
  log_stm.flush();
}
