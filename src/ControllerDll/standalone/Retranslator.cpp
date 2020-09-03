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

Retranslator::Retranslator(const std::string &source_port, const std::string &dst_port, uint32_t baudrate)
    : srcp(source_port, baudrate),
      dstp(dst_port, baudrate) {
  modificator = [](bytestring &s) {};
}

Retranslator::~Retranslator() {
  srcp.close();
  dstp.close();
}

void Retranslator::setModificator(std::function<void(bytestring &)> modificator) {
  this->modificator = modificator;
}

void Retranslator::start() {
  int ms_timeout = 100;
  dstp.setTimeout(serial::Timeout(0, ms_timeout));
  while (true) {
    bytestring bs{};
    size_t to_read = srcp.available();
    srcp.read(bs, to_read ? to_read : 1);

    //printf("Got command: ");
    //printHex(bs.data());
    //putchar('\n');

    // TEST
    // IF MULTIPLE COMMANDS WRITE LAST
    constexpr int commands_max_size = 50;
    uint8_t commands_uniq[50] = {};
    int size = parse_commands_unique(bs, commands_uniq, commands_max_size);
    //printf("Sending on terminal: ");
    //printHex(commands_uniq);
    //putchar('\n');

    dstp.write(commands_uniq, size);
    bs.clear();

    // before reading timeout
    // std::this_thread::sleep_for(std::chrono::milliseconds(ms_timeout));

    // otherwise it will not work in unison
    if (commands_uniq[size - 1] == 0x10)
        dstp.read(bs, 9);
    if (dstp.available())
        dstp.read(bs);  // read remaining bytes

    //printf("Read: ");
    //printf("%.*s", bs.size(), bs.data());
    //putchar('\n');

    modificator(bs);

    //printf("Returning: ");
    //printHex(bs.data());
    //putchar('\n');
    //putchar('\n');
    if (bs.size())
        srcp.write(bs);
  }
}