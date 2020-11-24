#include "../core/Output.hpp"
#include "../core/fixer.h"
#include "Retranslator.hpp"
#include "macro.hpp"

#include <Encode.hpp>
#include <Error.hpp>
#include <HookTypes.hpp>
#include <RetranslatorDefs.hpp>
#include <mmsg.hpp>

#include <SLogger.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string_view>

#include <Windows.h>

using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

static auto timePointLast = Time::now();
static ms timeSpanForStability =
    ms(500); // amount of time that weight shouldn't be changing to be stable
static unsigned int weight_delta_for_stability = 20;
static int lastWeight = 0;

static BOOL can_exit = FALSE;
static Retranslator *g_retranslator = nullptr;
static bool add_set_null_cmd = false;

#define IMPORT_DLL __declspec(dllimport)
extern "C" {
IMPORT_DLL void __stdcall setStatusErr(long status);
IMPORT_DLL void setEventHook(EventType event, SetHook *onSet);
}

static SLogger logger;
static bool enable_txt_logging = false;


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

static void setNullHook(long) {
  log("SetNullHook fired");
  add_set_null_cmd = true;
}

static void modifyRequest(bytestring &bs) {
  if (add_set_null_cmd) {
    add_set_null_cmd = false;
    log("Adding set NULL command");
    bs.push_back(0x0D);
  }
}

static size_t find_weight_response(const bytestring_view bsv) {
  size_t pos = bsv.rfind('=');
  if (pos != bytestring_view::npos &&
      bsv.size() - pos >= 9 - 1) // flag may be absent
  {
    // we have found '=' now we need to count how many number reserved bytes we
    // have if it's >=7 and all the chars there is numbers or ' ' | '$' | '%', ,
    // then return pos pointing on them otherwise return npos
    constexpr std::string_view alphabet = " -0123456789$%";
    // +8 and not +9, bcs we care only about numbers, $ or % flag is not in our
    // interest, though i leave them in alphabet
    for
      RANGE(pos + 1, pos + 7) {
        if (alphabet.find(static_cast<char>(bsv[i])) == std::string_view::npos)
          goto RETURN_NPOS;
      }
    return pos;
  }
RETURN_NPOS:
  return std::string::npos;
}

inline int extract_weight(size_t ans_start_pos, const bytestring_view bsv) {
  int res = 0;
  int sign = 1; // 1 for '+', -1 for '-'
  // skip '='
  // i in [1, 7]
  // in vesysoft, if given "1 2 3" result = 123, so we ignore spaces too
  for
    RANGE(ans_start_pos + 1, ans_start_pos + 8) {
      if (isdigit(bsv[i])) {
        res = res * 10 + CHAR_TO_INT(bsv[i]);
      } else if (bsv[i] == '-')
        sign = -1;
    }
  return res * sign;
}

// Weight values may be dirty so we need to decline them
// if prev_weight > || < curr_weight on 1000
// decline new weight
static void modify_resp(bytestring &bs) {
  static std::string prev_invalid = ""; // used at the end
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

      if ((unsigned int)std::abs(lastWeight - weight) <=
          weight_delta_for_stability) {
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
      printf("Fixed: %i\n\n", fixed);
      
      constexpr int last_stable_default = -1000;
      static int last_stable_fixed = last_stable_default;
      if (stable && last_stable_fixed == last_stable_default) {
        last_stable_fixed = fixed;
        log("Fixed %d", last_stable_fixed);
      } else if (weight < 1000) {
        last_stable_fixed = last_stable_default;
      }


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
      setStatusErr(ErrorCode::NoErrors);
      
      prev_invalid = "";
      return;
    }
  }
  std::string invalid_bytes = "";
  std::string invalid_bytes_numbers = "";
  for (auto bt : bs) {
    invalid_bytes += (char)bt;
  }
  if (prev_invalid != invalid_bytes) {
    log("Invalid bytes (%s)", invalid_bytes.c_str());
    prev_invalid = invalid_bytes;
  }
  setStatusErr(ErrorCode::ErrorInvalidData);
}

void initLogs(const std::string &logs_dir) {
  // create log directory, if needed
  char file_name[64] = {};

  const std::time_t now = std::time(nullptr);
  tm t;
  localtime_s(&t, &now);

  const char *format = "%Y_%m_%d %H_%M_%S Retranslator_log.txt";
  std::strftime(file_name, sizeof(file_name), format, &t);
  logger = SLogger(logs_dir + "\\" + file_name);
}

int main() {
#ifndef _DEBUG
  // constexpr auto iobuffer_size = 1024 * 16;
  // char iobuffer[iobuffer_size] = {};
  // std::setvbuf(stdout, iobuffer, _IOFBF, iobuffer_size);
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
  } catch (const ctrl::error &) {
    dprintf(msg<1>());
    std::exit(1);
  } catch (const std::runtime_error &e) {
    printf("Failed to decode: \"%s\"", e.what());
    dprintf(msg<2>());
    std::exit(1);
  }
  fclose(fp);

  auto str_ss = std::stringstream(ss);
  ini.parse(str_ss);

  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
    printf("Unnable to set console handler\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::exit(1);
  }

  init_fixer(ini);

  const std::string debug_console = ini.sections["DEBUG"]["LogLevel"];
  set_log_lvl(std::stoi(debug_console));
  setStatusErr(ErrorCode::ErrorNotReady);

  int log_lvl = get_log_lvl();
  if (log_lvl < 0 || log_lvl >= 2) {
    enable_txt_logging = true;

    const std::string logs_dir = current_dir + "\\logs";
    initLogs(logs_dir);
    log("Retranslator initing");
  }

  // start retranslator
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

    weight_delta_for_stability =
        std::stoi(ini.sections["DEFAULT"]["Weight-delta-for-stability"]);

    if (timeSpanForStability > ms(10000)) {
      dprintf(msg<0>());
      timeSpanForStability = ms(500);
    }
    auto r = Retranslator(src_port, dst_port);
    g_retranslator = &r;

    setEventHook(SetNull, setNullHook);

    // clear not ready error
    setStatusErr(ErrorCode::NoErrors);
    log("debug_lvl=%s", debug_console.c_str());
    log("src_port=%s", src_port.c_str());
    log("dst_port=%s", dst_port.c_str());
    log("ms_reading_timeout=%d ms", ms_reading_timeout);
    log("timeSpanForStability=%lli ms", timeSpanForStability.count());
    log("weight_delta_for_stability=%d kg", weight_delta_for_stability);
    log("Retranslator started. No errors");

    r.setModificator(modify_resp);
    r.setRequestModificator(modifyRequest);
    r.start(ms_reading_timeout);
  } catch (const serial::IOException &e) {
    if (debug_console == "0") {
      if (e.getErrorNumber() == 0) {
        MessageBoxW(NULL,
                    L"                          com       .                    "
                    L"             ,                                    ",
                    L"COM ERROR", MB_OK);
      } else {
        std::string str = e.what();
        std::wstring message = std::wstring(str.begin(), str.end());
        MessageBoxW(NULL, message.c_str(), L"COM ERROR", MB_OK);
        log("ERROR: %s", str.c_str());
      }
    } else {
      log("ERROR: %s", e.what());
      printf("%s\n", e.what());
      system("pause");
    }
  } catch (const std::exception &e) {
    if (debug_console == "0")
      dprintf(msg<3>());

    log("ERROR: %s", e.what());
    printf("%s\n", e.what());

    if (debug_console != "0")
      system("pause");
  }
  setStatusErr(ErrorCode::ErrorNotStarted);
  can_exit = TRUE;
  log("Exiting Retranslator");
  return 0;
}
