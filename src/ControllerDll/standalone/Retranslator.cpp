#include "Retranslator.hpp"
#include "Commands.hpp"
#include "macro.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <array>

static void printHex(bytestring_view s)
{
  for (uint8_t c : s)
    printf("0x%0.2x ", c);
}

static std::tuple<std::string, int> next_word(const std::string_view line, int offset = 0)
{
  int i = offset;
  // skip spaces
  for (; i < line.size() && isspace(line[i]); i++)
    ;
  // find len of the found word
  int len = 0;
  for (; i < line.size() && isalnum(line[i]); i++, len++)
    ;
  return std::tuple(
      std::string(std::cbegin(line) + i - len, std::cbegin(line) + i),
      i);
}

static void setup_serial(serial::Serial &com, const std::string_view settings)
{
#define IF_NEXT_WORD(code_block)                      \
  do                                                  \
  {                                                   \
    WordPos word_pos = next_word(settings, curr_pos); \
    std::string &word = std::get<0>(word_pos);        \
    if (std::get<1>(word_pos) != curr_pos)            \
      code_block else                                 \
      {                                               \
        com.open();                                   \
        return;                                       \
      }                                               \
    curr_pos = std::get<1>(word_pos);                 \
  } while (0);

  using WordPos = std::tuple<std::string, int>;
  int curr_pos = 0;

  IF_NEXT_WORD(
      com.setPort(word);)

  IF_NEXT_WORD(
      com.setBaudrate(std::stoi(word));)

  IF_NEXT_WORD(
      com.setBytesize(static_cast<serial::bytesize_t>(std::stoi(word)));)

  IF_NEXT_WORD({
    serial::parity_t parity = serial::parity_none; 
    if (word == "even")
      parity = serial::parity_even;
    else if (word == "odd")
      parity = serial::parity_odd;
    else if (word == "none")
      parity = serial::parity_none;
    com.setParity(parity);
  })
#undef IF_NEXT_WORD
}

Retranslator::Retranslator(const std::string &source_port,
                           const std::string &dst_port)
{
  setup_serial(dstp, dst_port);
  setup_serial(srcp, source_port);

  modificator = [](bytestring &s) {};
}

Retranslator::~Retranslator()
{
  srcp.close();
  dstp.close();
}

void Retranslator::setModificator(std::function<void(bytestring &)> modificator)
{
  this->modificator = modificator;
}

void Retranslator::start(int ms_timeout)
{
  dstp.setTimeout(serial::Timeout(0, ms_timeout));

  while (run_flag)
  {
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

    constexpr auto max_read = 9;
    dstp.read(bs, max_read); // he needs time around 10-50ms to write operation, and we give that time via timeout on reading

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
}