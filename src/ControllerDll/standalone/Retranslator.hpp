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
  const std::string& dst_port, 
  const std::string& bridge_port);
  ~Retranslator();

  // can be set only once
  void setModificator(std::function<void(bytestring &)> modificator);
  void start(int ms_timeout = 50);

private:
  void bridgeRun(int ms_bridgep_timeout);

private:
  serial::Serial srcp;
  serial::Serial dstp;
  serial::Serial bridgep; // for controller, no modification applied
  std::function<void(bytestring &)> modificator;

  std::atomic_flag is_running_flag = ATOMIC_FLAG_INIT;
  std::mutex dstp_mut;
};