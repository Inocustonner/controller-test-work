#pragma once
#include "bytestring.hpp"

#include <serial/serial.h>
#include <string>
#include <vector>
#include <functional>


class Retranslator
{
public:
  Retranslator() = delete;
  Retranslator(const std::string &source_port, const std::string &dst_port, uint32_t baudrate = 9600U);
  ~Retranslator();

  // can be set only once
  void setModificator(std::function<void(bytestring &)> modificator);
  void start(int ms_timeout = 100);

private:
  serial::Serial srcp;
  serial::Serial dstp;
  std::function<void(bytestring &)> modificator;
};