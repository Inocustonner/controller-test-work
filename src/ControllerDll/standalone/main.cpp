#include "../core/Output.hpp"
#include "../core/fixer.h"
#include "Retranslator.hpp"
#include "macro.hpp"
#include <Encode.hpp>
#include <Error.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string_view>

#include <atomic>

#include <Windows.h>

using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

static auto timePointLast = Time::now();
static ms timeSpanForStability =
    ms(500); // amount of time that weight shouldn't be changing to be stable
static int lastWeight = 0;

static BOOL can_exit = FALSE;
static Retranslator *g_retranslator = nullptr;

#define IMPORT_DLL __declspec(dllimport)
extern "C" {
IMPORT_DLL void __stdcall setStatus(long status);
}

BOOL WINAPI CtrlHandler(DWORD signal) {
  if (signal == CTRL_C_EVENT) {
    printf("Closing retranslator...n");
    g_retranslator->run_flag = false;
    while (!can_exit) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return TRUE;
  } else
    return FALSE;
}

static size_t find_weight_response(const bytestring_view bsv) {
  size_t pos = bsv.rfind('=');
  if (pos != bytestring_view::npos &&
      bsv.size() - pos >= 9 - 1) // flag may be absent
  {
    // we have found '=' now we need to count how many number reserved bytes we
    // have if it's >=7 and all the chars there is numbers or ' ' | '$' | '%', ,
    // then return pos pointing on them otherwise return npos
    constexpr std::string_view alphabet = " 0123456789$%";
    // +8 and not +9, bcs we care only about numbers, $ or % flag is not in our
    // interest, though i leave them in alphabet
                for
                  RANGE(pos + 1, pos + 7) {
                    if (alphabet.find(static_cast<char>(bsv[i])) ==
                        std::string_view::npos)
                      goto RETURN_NPOS;
                  }
                return pos;
  }
RETURN_NPOS:
  return std::string::npos;
}

inline int extract_weight(size_t ans_start_pos, const bytestring_view bsv) {
  int res = 0;
  // skip first 2 syms "=(-| )", because vesysoft don't use them
  // i in [2, 7]
  // in vesysoft, if given "1 2 3" result = 123, so we ignore spaces too
        for
          RANGE(ans_start_pos + 2, ans_start_pos + 8) {
            if (isdigit(bsv[i])) {
              res = res * 10 +
                    CHAR_TO_INT(bsv[i]); // bcs answer '33 $= 1345' is still
                                         // valid we need to search circular
            }
          }
        return res;
}

// Weight values may be dirty so we need to decline them
// if prev_weight > || < curr_weight on 1000
// decline new weight
static void modify_resp(bytestring &bs) {
  if (bs.size() >= 9) { // when weight is returned, bs size == 9, if it's bigger
                        // there is some litter
    // valid weight response have the following scheme
    // =(-| )(\d| ){6}($|%)
    // last part, can be skipped
    size_t ans_start_pos =
        find_weight_response(bytestring_view{bs.data(), bs.size()});
    if (ans_start_pos != std::string::npos) {
      // printf("RESPONSE VALID AND STARTS FROM: %u\n", ans_start_pos);
      const int weight =
          extract_weight(ans_start_pos, bytestring_view{bs.data(), bs.size()});
      bool stable = false;

      if (lastWeight == weight) {
        auto timeCurrentPoint = Time::now();
        if (timeCurrentPoint - timePointLast >= timeSpanForStability) {
          stable = true;
        }
      } else {
        timePointLast = Time::now();
      }

      int fixed = fix(weight, stable);
      printf("Extracted weight: %i[%s]\n", weight,
             stable ? "Stable" : "Unstable");
      printf("Fixed: %i\n", fixed);

      // bcs weights use 6 bytes as with decimal in each, 2nd byte reserved for
      // '-'
      if (fixed < 999999) {
        uint8_t flag = *bs.rbegin();
        bs.erase(std::end(bs) - 8, std::end(bs));

        std::vector<char> buf(8, '0');
        std::sprintf(buf.data(), "%0.6i", fixed);
        *buf.rbegin() = flag;

        // bcs sprintf inserts '\0'
        if (fixed >= 0)
          *(buf.rbegin() + 1) = ' ';

        bs.insert(std::end(bs), std::cbegin(buf), std::cend(buf));
      }
      lastWeight = weight;
    }
  }
}

int main() {
#ifndef _DEBUG
  constexpr auto iobuffer_size = 1024 * 16;
  char iobuffer[iobuffer_size] = {};
  std::setvbuf(stdout, iobuffer, _IOFBF, iobuffer_size);
#endif
  // find current_dir
  char path_buf[MAX_PATH + 1] = {};
  GetModuleFileNameA(NULL, reinterpret_cast<char *>(path_buf),
                     std::size(path_buf) - 1);
  auto current_dir = std::string(path_buf);
  current_dir.erase(std::begin(current_dir) + current_dir.find_last_of('\\') +
                        1,
                    std::end(current_dir));

  // load ini file
  constexpr auto ini_name = "ini.ini";
  const std::string path_to_ini = current_dir + ini_name;

  inipp::Ini<char> ini;
  FILE *fp = nullptr;

  std::string ss;
  try {
    fp = fopen(path_to_ini.c_str(), "rb");
    if (!fp) {
      throw ctrl::error("Failed to open file %s\n", path_to_ini.c_str());
    }
    fdecode(ss, fp);
  } catch (const ctrl::error &e) {
    dprintf(msg<1>());
    std::exit(1);
  } catch (const std::runtime_error &e) {
    printf("Failed to decode: \"%s\"", e.what());
    dprintf(msg<2>());
    std::exit(1);
  }
  fclose(fp);

  ini.parse(std::stringstream(ss));

  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
    printf("Unnable to set console handler\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::exit(1);
  }

  init_fixer(ini);

  // start retranslator
  const std::string debug_console = ini.sections["DEBUG"]["LogLevel"];
  try {
    if (debug_console == "0") {
      ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    }
    const std::string src_port =
        ini.sections["DEFAULT"]["COM-end-for-retranslator"];
    const std::string dst_port = ini.sections["DEFAULT"]["COM-for-weights"];
    const int ms_reading_timeout =
        std::stoi(ini.sections["DEFAULT"]["Weights-reading-timeout"]);
    timeSpanForStability =
        ms(std::stoi(ini.sections["DEFAULT"]["Time-span-for-stability-ms"]));
    if (timeSpanForStability > ms(10000)) {
      dprintf(msg<0>());
      timeSpanForStability = ms(500);
    }
    auto r = Retranslator(src_port, dst_port);
    g_retranslator = &r;
    r.setModificator(modify_resp);
    r.start(ms_reading_timeout);
  } catch (const std::exception &e) {
    if (debug_console == "0")
        dprintf(msg<3>());

    printf("%s\n", e.what());

    if (debug_console != "0")
        system("pause");
  }
  can_exit = TRUE;
  return 0;
}
