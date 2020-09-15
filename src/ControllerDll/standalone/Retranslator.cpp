#include "Retranslator.hpp"
#include "Commands.hpp"
#include "macro.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <array>

static void printHex(bytestring_view s) {
  for (uint8_t c : s)
    printf("0x%0.2x ", c);
}

Retranslator::Retranslator(const std::string& source_port, 
  const std::string& dst_port, 
  const std::string& bridge_port,
  uint32_t baudrate)
    : srcp(source_port, baudrate),
      dstp(dst_port, baudrate),
      bridgep(bridge_port, baudrate) {
  modificator = [](bytestring &s) {};
}

Retranslator::~Retranslator() {
  srcp.close();
  dstp.close();
  bridgep.close();
}

void Retranslator::setModificator(std::function<void(bytestring &)> modificator) {
  this->modificator = modificator;
}

void Retranslator::bridgeRun(int ms_bridgep_timeout) {
  bridgep.setTimeout(serial::Timeout(0, ms_bridgep_timeout));
  constexpr auto max_read = 1000;
  while (is_running_flag.test_and_set(std::memory_order_acquire)) {
    bytestring bs{};
    bridgep.read(bs, max_read);

    dstp_mut.lock();

    constexpr int commands_max_size = 50;
    uint8_t commands_uniq[50] = {};
    int size = parse_commands_unique(bs, commands_uniq, commands_max_size);

    dstp.write(commands_uniq, size);

    bs.clear();
    dstp.read(bs, max_read);

    dstp_mut.unlock();
    bridgep.write(bs.data(), bs.size());
  }
}

void Retranslator::start(int ms_timeout) {
  dstp.setTimeout(serial::Timeout(0, ms_timeout));
  is_running_flag.test_and_set(std::memory_order_acquire);

  auto bridge_thread = std::thread(std::bind(&Retranslator::bridgeRun, this, ms_timeout));
  while (true) {
    bytestring bs{};
    size_t to_read = srcp.available();
    srcp.read(bs, std::max(to_read, static_cast<size_t>(1)));

    if (bs.size() == 0)
      continue;
    //printf("Got command: ");
    //printHex(bs.data());
    //putchar('\n');

    constexpr int commands_max_size = 50;
    uint8_t commands_uniq[50] = {};
    int size = parse_commands_unique(bs, commands_uniq, commands_max_size);
    //printf("Sending on terminal: ");
    //printHex(commands_uniq);
    //putchar('\n');

    dstp_mut.lock();

    dstp.write(commands_uniq, size);
    bs.clear();

    dstp.read(bs, 1000); // he needs time around 10-50ms to write operation, and we give that time via timeout on reading
    
    dstp_mut.unlock();

    printf("Read: ");
    printf("%.*s", bs.size(), bs.data());
    putchar('\n');

    modificator(bs);

    //printf("Returning: ");
    //printHex(bs.data());
    //putchar('\n');
    //putchar('\n');
    if (bs.size())
        srcp.write(bs);
  }
  is_running_flag.clear(std::memory_order_release);
}