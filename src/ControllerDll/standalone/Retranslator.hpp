#pragma once
#include "bytestring.hpp"

#include <serial/serial.h>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

class Retranslator
{
public:
  Retranslator() = delete;
  Retranslator(const std::string& source_port, 
  const std::string& dst_port);
  ~Retranslator();

  // can be set only once
  void setModificator(std::function<void(bytestring &)> modificator);
  void start(int ms_timeout = 50);
  bool run_flag = true;

private:
  serial::Serial srcp;
  serial::Serial dstp;
  std::function<void(bytestring &)> modificator;

  std::mutex dstp_mut;
};