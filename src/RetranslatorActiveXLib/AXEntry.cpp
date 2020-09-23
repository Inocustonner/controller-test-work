#include <Windows.h>
#include <new>
#include <atomic>
#include "Factory.hpp"

HMODULE g_module = NULL;
std::atomic_long g_objsInUse = 0;

#pragma data_seg(".data_shared")
extern "C" {
  alignas(sizeof(long)) volatile long weightRaw = 0;
  alignas(sizeof(long)) volatile long weightFixed = 0;
}
#pragma data_seg()

extern "C" {
  void __stdcall setWeight(long weight) {
      InterlockedExchange(&weightRaw, weight);
  }
  void __stdcall setWeightFixed(long weight) {
      InterlockedExchange(&weightFixed, weight);
  }
}


STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv) {
  if (clsid == CLSID_RetranslatorAX) {
    Factory* factory = new (std::nothrow) Factory{};
    if (factory)
      return factory->QueryInterface(iid, ppv);
    else
      return E_OUTOFMEMORY;
  }
  else
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() {
  if (g_objsInUse == 0)
    return S_OK;
  else
    return S_FALSE;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		g_module = hModule;
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
		break;
    }
    return TRUE;
}
