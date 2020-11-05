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

#define IMPORT_DLL __declspec(dllimport)

extern "C" {
IMPORT_DLL void initDll();
IMPORT_DLL void setEventHook(EventType event, SetHook *onSet);
IMPORT_DLL void fireEvent(EventType event);
IMPORT_DLL void __stdcall setWeight(long weight);
IMPORT_DLL void __stdcall setWeightFixed(long weight);

IMPORT_DLL void __stdcall setStatusStability(bool stability);
IMPORT_DLL void __stdcall setStatusAuth(bool auth);
}

void onSetCorr(long corr) {
  if (-100 < corr && corr < 100) {
    const double corr_koef = corr / 100.0;
    state.corr = [&min_w = state.min_weight,
                  corr_koef](comptype inp_w) -> comptype {
      return static_cast<comptype>((inp_w - min_w) * corr_koef) + inp_w;
    };
  } else {
    state.corr = [corr_w = corr](comptype inp_w) -> comptype {
      return (inp_w + corr_w);
    };
  }
}

void onSetMinWeight(long new_min_w) {
  state.min_weight = new_min_w;
  state.authorized = true;

  reset_thr = state.min_weight / 2;
}

void onClearAuth(long) {
  state.authorized = false;
}

inline comptype phase1(const comptype p1, const bool is_stable) {
  comptype corr_weight = state.corr(p1);
  state.passed_upper_gate = true;
  if (is_stable) {
    state.p0s = p1;
  }
  return corr_weight;
}

static comptype delta = 200;

inline void phase0(const comptype p1) {
  if (state.p0 > reset_thr && reset_thr > p1 && state.passed_upper_gate) {
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
  if (get_log_lvl() > 0)
    printf("Authorization: %s\n", state.authorized ? "true" : "false");
  if (state.authorized) {
    if (p1 < state.min_weight)
      phase0(p1);
    else
      ret_value = phase1(p1, is_stable);
  } 
  state.p0 = p1;

  setWeight(p1);
  setWeightFixed(ret_value);

  setStatusAuth(state.authorized);
  setStatusStability(is_stable);

  return ret_value;
}

bool init_fixer(const inipp::Ini<char> &ini) {
  initDll();
  setEventHook(SetMinimalWeight, onSetMinWeight);
  setEventHook(SetCorr, onSetCorr);
  setEventHook(ClearAuth, onClearAuth);
  fireEvent(SetCorr); // if corr was set before loading retranslator, update it here
  try {
    init_settings(ini);
  } catch (const ctrl::error &e) {
    printf(e.what());
    if (get_log_lvl() > 1)
      std::system("pause");
    std::exit(1);
  }
  return true;
}
