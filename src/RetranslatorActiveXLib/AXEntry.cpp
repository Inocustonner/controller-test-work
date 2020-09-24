#include <Windows.h>
#include <new>
#include <atomic>
#include "Factory.hpp"
#include "Hook.hpp"

HMODULE g_module = NULL;
std::atomic_long g_objsInUse = 0;

#define ALIGN32 alignas(32)
#define SHARED_VAR ALIGN32 volatile

#pragma data_seg(".data_shared")
extern "C"
{
  SHARED_VAR long g_weightRaw = 0;
  SHARED_VAR long g_weightFixed = 0;
  SHARED_VAR long g_status = 0; // 0003 - com port error, 10** - authorized, 20** - unauthorized, **01 - stable, **02 - unstable

  SHARED_VAR long g_minWeight = 0;
  SHARED_VAR long g_corr = 0;
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
  void __stdcall setStatus(long status)
  {
    InterlockedExchange(&g_status, status);
  }
}

STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv)
{
  if (clsid == CLSID_RetranslatorAX)
  {
    Factory *factory = new (std::nothrow) Factory{};
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
