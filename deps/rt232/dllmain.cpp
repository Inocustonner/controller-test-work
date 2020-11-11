#include <Windows.h>
#include <new>

#include "Component.rt232.hpp"
#include "Factory.hpp"

HMODULE g_module = NULL;
std::atomic_long g_objsInUse = 0;

STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv)
{
  if (clsid == CLSID_Rt232)
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

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
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
