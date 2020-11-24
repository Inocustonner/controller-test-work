#include <Windows.h>
#include <new>
#include <atomic>
#include "Factory.hpp"
#include "Hook.hpp"
#include "include/RetranslatorDefs.hpp"

HMODULE g_module = NULL;
std::atomic_long g_objsInUse = 0;

#define ALIGN32 alignas(32)
#define ALIGN64 alignas(64)
#define SHARED_VAR ALIGN32 volatile
#define SHARED_VA64 ALIGN64 volatile

#define STATE_0 (ErrorNotStarted << 2)

#pragma data_seg(".data_shared")

extern "C"
{
  SHARED_VAR long g_weightRaw = 0;
  SHARED_VAR long g_weightFixed = 0;
  SHARED_VAR Status g_status = Status{ .f_long = STATE_0 };

  constexpr long default_max_weight = 999999;
  SHARED_VAR long g_maxWeight = default_max_weight;
  SHARED_VAR long g_minWeight = 0;
  SHARED_VAR long g_corr = 0;

  SHARED_VAR unsigned long created_process_pid = -1;
  SHARED_VA64 double g_reset_thr = 2.5;
}
#pragma data_seg()

extern "C"
{
  void __stdcall setWeight(long weight)
  {
    InterlockedExchange(&g_weightRaw, weight);
  }
  void __stdcall setWeightFixed(long weight)
  {
    InterlockedExchange(&g_weightFixed, weight);
  }
  void __stdcall setStatusErr(long err_code)
  {
    Status s;
    InterlockedReadStatus(&s, &g_status);
    s.err = err_code;
    
    InterlockedSetStatus(&g_status, s);
  }
  void __stdcall setStatusStability(bool stability) {
    Status s;
    InterlockedReadStatus(&s, &g_status);
    s.stability = (int)stability;

    InterlockedSetStatus(&g_status, s);
  }
  void __stdcall setStatusAuth(bool auth) {
    Status s;
    InterlockedReadStatus(&s, &g_status);
    s.auth = (int)auth;

    InterlockedSetStatus(&g_status, s);
  }
  void __stdcall clearStatus() {
    Status s = {};
    InterlockedSetStatus(&g_status, s);
  }
  double __stdcall getResetThrKoef() {
    return g_reset_thr; // NOT INTERLOCKED
  }
}

STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv)
{
  if (clsid == CLSID_RetranslatorAX || clsid == CLSID_RetranslatorUtilsAX)
  {
    Factory *factory = new (std::nothrow) Factory{clsid};
    if (factory)
      return factory->QueryInterface(iid, ppv);
    else
      return E_OUTOFMEMORY;
  }
  else
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
  if (g_objsInUse == 0)
    return S_OK;
  else
    return S_FALSE;
}

extern "C"
void initDll() {
  startListener();
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    g_module = hModule;
    initListener();
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    stopListener();
    break;
  }
  return TRUE;
}
