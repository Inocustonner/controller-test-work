#pragma once
#include "bytestring.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <serial/serial.h>
#include <string>
#include <vector>

#define IMPORT_DLL __declspec(dllimport)
extern "C" {
IMPORT_DLL void __stdcall setStatus(long status);
}

class Retranslator {
public:
  Retranslator() = delete;
  Retranslator(const std::string &source_port, const std::string &dst_port);
  ~Retranslator();

  // can be set only once
  void setModificator(std::function<void(bytestring &)> modificator);
  void
  setRequestModificator(std::function<void(bytestring &)> request_modificator);
  void start(int ms_timeout = 50);
  bool run_flag = true;

private:
  serial::Serial srcp;
  serial::Serial dstp;
  std::function<void(bytestring &)> modificator;
  std::function<void(bytestring &)> request_modificator;
};
