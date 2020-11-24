#include "Hook.hpp"

#include <Windows.h>
#include <thread>

#define WAIT_ONE FALSE

#define InterlockedRead(var) InterlockedExchangeAdd(&(var), 0)

extern "C" volatile long g_maxWeight;
extern "C" volatile long g_minWeight;
extern "C" volatile long g_corr;
extern "C" volatile double g_reset_thr;

static void set_def_(long) {}
static SetHook* onSetMaximalWeightHook = set_def_;
static SetHook* onSetMinimalWeightHook = set_def_;
static SetHook *onSetCorrHook = set_def_;
static SetHook* onSetNullHook = set_def_;
static SetHook* onClearAuthHook = set_def_;
static SetHook* onSetResetThrKoef = set_def_;

static HANDLE events[EventsCnt] = {};

static bool stop = true;
static std::thread listener_th;

extern "C" {

void fireEvent(EventType event) { SetEvent(events[event]); }

void setEventHook(EventType event, SetHook *onSet) {
  switch (event) {

  case SetMaximalWeight: 
    onSetMaximalWeightHook = onSet;
    break;

  case SetMinimalWeight:
    onSetMinimalWeightHook = onSet;
    break;

  case SetCorr:
    onSetCorrHook = onSet;
    break;

  case SetNull:
    onSetNullHook = onSet;
    break;

  case ClearAuth:
    onClearAuthHook = onSet;
    break;
  case SetResetThr:
    onSetResetThrKoef = onSet;
    break;
  }
}
}

static HANDLE createEvent(const char *name) {
  HANDLE h = CreateEventA(NULL, FALSE, FALSE, name);
  if (h == INVALID_HANDLE_VALUE) {
    throw std::exception("Error creating event");
  }
  return h;
}

static void listenFunc() {
  while (!stop) {
    EventType event_t = static_cast<EventType>(
        WaitForMultipleObjects(EventsCnt, events, WAIT_ONE, INFINITE));
    switch (event_t) {

    case SetMaximalWeight: {
      onSetMaximalWeightHook(InterlockedRead(g_maxWeight));
      ResetEvent(events[event_t]);
      break;
    }

    case SetMinimalWeight: {
      onSetMinimalWeightHook(InterlockedRead(g_minWeight));
      ResetEvent(events[event_t]);
      break;
    }

    case SetCorr: {
      onSetCorrHook(InterlockedRead(g_corr));
      ResetEvent(events[event_t]);
      break;
    }
    case SetNull: {
      onSetNullHook(0);
      ResetEvent(events[event_t]);
      break;
    }
    case ClearAuth: {
      onClearAuthHook(0);
      ResetEvent(events[event_t]);
      break;
    }
    case SetResetThr: {
      onSetResetThrKoef(0);
      ResetEvent(events[event_t]);
    }break;
    }
  }
}

void initListener() {
  events[SetMaximalWeight] = createEvent("SetMaximalWeightEvent");
  events[SetMinimalWeight] = createEvent("SetMinimalWeightEvent");
  events[SetCorr] = createEvent("SetCorr");
  events[SetNull] = createEvent("SetNull");
  events[ClearAuth] = createEvent("ClearAuth");
  events[SetResetThr] = createEvent("SetResetThr");
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
