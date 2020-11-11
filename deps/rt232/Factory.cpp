#include "Component.rt232.hpp"
#include "Factory.hpp"

extern std::atomic_long g_objsInUse;

Factory::Factory() { g_objsInUse++; }

Factory::~Factory() { g_objsInUse--; }

HRESULT __stdcall Factory::QueryInterface(REFIID riid, void **ppv) {
  if (IsEqualGUID(riid, IID_IClassFactory)) {
    AddRef();
    *ppv = this;
    return S_OK;
  } else {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG __stdcall Factory::AddRef() { return ++m_refCount; }

ULONG __stdcall Factory::Release() {
  long res = --m_refCount;
  if (res == 0)
    delete this;
  return res;
}

HRESULT __stdcall Factory::CreateInstance(IUnknown *outer, const IID &iid,
                                          void **ppv) {
  if (outer != nullptr) {
    return CLASS_E_NOAGGREGATION;
  }

  IUnknown *cretr = nullptr;
  cretr = static_cast<IInitDone*>(new (std::nothrow) Rt232);
  
  if (cretr == nullptr) {
    return E_OUTOFMEMORY;
  }
  return cretr->QueryInterface(iid, ppv);
}

HRESULT __stdcall Factory::LockServer(BOOL lock) { return E_NOTIMPL; }
