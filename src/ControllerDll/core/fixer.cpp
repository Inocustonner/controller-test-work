#include <HookTypes.hpp>

#include "Init.hpp"
#include "Output.hpp"
#include "State.hpp"
#include "mmsg.hpp"

#include <Error.hpp>

#include <cstdio>
#include <cstdlib>
#include <thread>

#include <RetranslatorDefs.hpp>

#include "fixer.h"
#include <SLogger.hpp>

#define IMPORT_DLL __declspec(dllimport)

extern "C" {
IMPORT_DLL void initDll();
IMPORT_DLL void setEventHook(EventType event, SetHook *onSet);
IMPORT_DLL void fireEvent(EventType event);
IMPORT_DLL void __stdcall setWeight(long weight);
IMPORT_DLL void __stdcall setWeightFixed(long weight);

IMPORT_DLL void __stdcall setStatusStability(bool stability);
IMPORT_DLL void __stdcall setStatusAuth(bool auth);
IMPORT_DLL double __stdcall getResetThrKoef();
}

extern void log(const char* format, ...);

void onSetCorr(long corr) {
  log("SetCorr %d", corr);
  if (-100 < corr && corr < 100) {
    const double corr_koef = corr / 100.0;
    state.corr = [&min_w = state.min_weight,
                  corr_koef](comptype inp_w) -> comptype {
      return (static_cast<comptype>((inp_w - min_w) * corr_koef) + inp_w);
    };
  } else {
    state.corr = [corr_w = corr](comptype inp_w) -> comptype {
      return (inp_w + corr_w);
    };
  }
}

void onSetMinWeight(long new_min_w) {
  log("SetMinWeight %d", new_min_w);
  state.min_weight = new_min_w;
  state.authorized = true;

  state.reset_thr = static_cast<comptype>(state.min_weight / reset_thr_koef);
}

void onSetMaxWeight(long new_max_w) {
  log("SetMaxWeight %d", new_max_w);
  state.max_weight = new_max_w;
}

void onSetResetThr(long) {
  log("SetResetThr");
  reset_thr_koef = getResetThrKoef();
  state.reset_thr = static_cast<comptype>(state.min_weight / reset_thr_koef);
}

void onClearAuth(long) {
  log("ClearAuth");
  state.authorized = false;
}

inline comptype phase1(const comptype p1, const bool is_stable) {
  comptype corr_weight = state.corr(p1);

  state.reset_thr = std::max(static_cast<comptype>(p1 / reset_thr_koef), state.reset_thr);

  state.passed_upper_gate = true;
  if (is_stable) {
    state.p0s = p1;
  }
  return corr_weight;
}

static comptype delta = 200;

inline void phase0(const comptype p1) {
  if (state.p0 > state.reset_thr && state.reset_thr > p1 && state.passed_upper_gate) {
    printf("resetting\n");
    if (get_log_lvl() > 1) {
      wchar_t buffer_msg[100] = {};
      swprintf(buffer_msg, std::size(buffer_msg),
               L"\"Resetting state\n↓%d\n- - - -< %d >- - - -\n%d\n\"",
               state.p0, reset_thr, p1);
      mMsgBox(L"INFO", buffer_msg, get_msg_duration());
    }
    reset_state();
  }
}

comptype fix(const comptype p1, const bool is_stable) {
  comptype ret_value = p1;
  if (state.authorized) {
    if (p1 < state.reset_thr)
      phase0(p1);
    else if (p1 >= state.min_weight && p1 < state.max_weight)
      ret_value = phase1(p1, is_stable);
    else if (p1 >= state.max_weight)
      ret_value = state.max_weight - p1 % 100;
  } 
  if (get_log_lvl() > 0) {
    printf("Authorization: %s\n", state.authorized ? "true" : "false");
    printf("Reset thr: %d\n", state.reset_thr);
  }  
  state.p0 = p1;

  ret_value = ret_value / rounding * rounding ;

  setWeight(p1);
  setWeightFixed(ret_value);

  setStatusAuth(state.authorized);
  setStatusStability(is_stable);

  return ret_value;
}

bool init_fixer(const inipp::Ini<char> &ini) {
  initDll();
  log("Setting hooks");
  setEventHook(SetMaximalWeight, onSetMaxWeight);
  setEventHook(SetMinimalWeight, onSetMinWeight);
  setEventHook(SetCorr, onSetCorr);
  setEventHook(ClearAuth, onClearAuth);
  setEventHook(SetResetThr, onSetResetThr);
  fireEvent(SetCorr); // if corr was set before loading retranslator, update it here
  try {
    init_settings(ini);
  } catch (const ctrl::error &e) {
    log("ERROR: %s", e.what());
    printf(e.what());
    if (get_log_lvl() > 1)
      std::system("pause");
    std::exit(1);
  }
  return true;
}
