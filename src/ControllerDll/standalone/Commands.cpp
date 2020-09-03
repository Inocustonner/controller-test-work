#include "Commands.hpp"
#include "macro.hpp"

#include <algorithm>
#include <array>

constexpr std::uint8_t operator"" _u8(unsigned long long value)
{
  return static_cast<std::uint8_t>(value);
}

static const std::array default_commands = {
    //0x01_u8,
    0x10_u8,
    0x11_u8,
    0x19_u8,
    0x14_u8,
    //0x13_u8
    0x15_u8,
    //0x12_u8
    0x18_u8,
    0x0D_u8,
    0x16_u8,
    0x17_u8,
    0x02_u8};

template <typename T, class Container>
static bool in(const Container &cont, const T val)
{
  if (std::find(std::cbegin(cont), std::cend(cont), val) != std::cend(cont))
    return true;
  else
    return false;
}

/* return iter on elem */
template <class Container, class UnaryPred>
static auto find_pred(Container& cont, UnaryPred pred) {
  for ITER_RANGE(std::begin(cont), std::end(cont)) {
    if (pred(*it))
      return it;
  }
  return std::end(cont);
}

int parse_commands_unique(const bytestring &bs, uint8_t* dst_commands_uniq, int max_size)
{
  size_t pos = 0;
  auto match = [&pos, &bs](uint8_t b) { return bs[pos] == b; };
  
  /* std::tuple(cmd, last_pos) */
  std::vector<std::tuple<bytestring_view, size_t>> commands = {};
  size_t prev_cmd_len = 0;
  for (; pos < bs.size(); pos++)
  {
    size_t cmd_len = 1;
    // if current command not in commands
    auto it = find_pred(commands,
                        [cmd = bs[pos]](auto bs_sz_tup) { return std::get<0>(bs_sz_tup).data()[0] == cmd; });
    if (it != std::end(commands))
    {
      /* update last position  */
      std::get<1>(*it) = pos;
    }
    else
    {
      /* only 3 commands have command len > 1 */
      if (match(0x01))
        cmd_len = 5;
      else if (match(0x12))
        cmd_len = 9;
      else if (match(0x13))
        cmd_len = 2;
      else if (in(default_commands, bs[pos]))
        cmd_len = 1;
      else
        continue; // if it isn't command, skip it

      commands.push_back({ bytestring_view{bs.data() + pos, cmd_len}, pos});
    }
    prev_cmd_len = cmd_len;
    pos += cmd_len - 1;
  }
  /* Put commands in order in new vector and return*/
  // sort -> latest last
  std::sort(std::begin(commands), std::end(commands), [](auto t1, auto t2) { return std::get<1>(t1) < std::get<1>(t2); });

  int written = 0;
  for ITER_RANGE(std::cbegin(commands), std::cend(commands)) {
    auto cmd = std::get<0>(*it);
    // if we have a space for a entire command
    if (cmd.size() < max_size - written) {
      std::memcpy(dst_commands_uniq + written, cmd.data(), cmd.size());
      written += cmd.size();
    }
    else break;
  }
  return written;
}