#include "Hook.hpp"

#include <Windows.h>
#include <functional>
#include <magic_enum.hpp>
#include <thread>

#define WAIT_ONE FALSE

#define InterlockedRead(var) InterlockedExchangeAdd(&(var), 0)

extern "C" volatile long g_maxWeight;
extern "C" volatile long g_minWeight;
extern "C" volatile long g_corr;
extern "C" volatile double g_reset_thr;

// must match with 'EventType' enum
static std::tuple hookFuncs = {
    (HookSet) nullptr, //SetMax
    (HookSet) nullptr, //SetMin
    (HookSet) nullptr, //SetCorr
    (HookNotify) nullptr, //SetNull
    (HookNotify) nullptr, //SetResetThr
    (HookNotify) nullptr, // ClearAuth
};

static HANDLE events[magic_enum::enum_count<EventType>()] = {};

static bool stop = true;
static std::thread listener_th;

template <class Func, class Tuple, size_t N = 0>
inline void runtime_get(Func func, Tuple &tup, size_t idx) {
  if (N == idx) {
    func(std::get<N>(tup));
    return;
  }

  if constexpr (N + 1 < std::tuple_size_v<Tuple>) {
    return runtime_get<Func, Tuple, N + 1>(func, tup, idx);
  }
}

extern "C" {

  void fireEvent(EventType event) { SetEvent(events[event]); }

  void setEventHook(EventType event, void* onSet) {
#pragma message("Enable checks for type of 'onSet' somewhere")
    runtime_get(
      [onSet](auto& hook_f) {
        hook_f = (std::remove_reference_t<decltype(hook_f)>)onSet;
      },
      hookFuncs, event);
  }
}
static HANDLE createEvent(const char* name) {
  HANDLE h = CreateEventA(NULL, FALSE, FALSE, name);
  if (h == INVALID_HANDLE_VALUE) {
    throw std::exception("Error creating event");
  }
  return h;
}

static void listenFunc() {
  while (!stop) {
    EventType event_t = static_cast<EventType>(
      WaitForMultipleObjects(std::size(events), events, WAIT_ONE, INFINITE));

#define CALL_CASE_FOR(enum_value, call)                                        \
case EventType::enum_value: {                                                \
  auto f_ptr = std::get<EventType::enum_value>(hookFuncs);                   \
  if (f_ptr != 0)                                                            \
    f_ptr call;                                                              \
}break

    switch (event_t) {

      CALL_CASE_FOR(SetMaximalWeight,
        (InterlockedRead(g_maxWeight)));

      CALL_CASE_FOR(SetMinimalWeight,
        (InterlockedRead(g_minWeight)));

      CALL_CASE_FOR(SetCorr,
        (InterlockedRead(g_corr)));

      CALL_CASE_FOR(SetNull,
        ());

      CALL_CASE_FOR(ClearAuth,
        ());

      CALL_CASE_FOR(SetResetThr,
        ());

      //case SetMaximalWeight: {
      //  onSetMaximalWeightHook(InterlockedRead(g_maxWeight));
      //  break;
      //}

      //case SetMinimalWeight: {
      //  onSetMinimalWeightHook(InterlockedRead(g_minWeight));
      //  break;
      //}

      //case SetCorr: {
      //  onSetCorrHook(InterlockedRead(g_corr));
      //  break;
      //}
      //case SetNull: {
      //  onSetNullHook(0);
      //  break;
      //}
      //case ClearAuth: {
      //  onClearAuthHook(0);
      //  break;
      //}
      //case SetResetThr: {
      //  onSetResetThrKoef(0);
      //} break;
    default:
      continue;
    }
    ResetEvent(events[event_t]);
#undef CALL_CASE_FOR
  }
}

void initListener() {
  for (const auto& [index, name] : magic_enum::enum_entries<EventType>()) {
    events[index] = createEvent(name.data());
  }
  // events[SetMaximalWeight] = createEvent("SetMaximalWeightEvent");
  // events[SetMinimalWeight] = createEvent("SetMinimalWeightEvent");
  // events[SetCorr] = createEvent("SetCorr");
  // events[SetNull] = createEvent("SetNull");
  // events[ClearAuth] = createEvent("ClearAuth");
  // events[SetResetThr] = createEvent("SetResetThr");
}

void startListener() {
  stop = false;
  listener_th = std::thread(listenFunc);
}

void stopListener() {
  stop = true;
  if (listener_th.joinable())
    listener_th.join();
}
