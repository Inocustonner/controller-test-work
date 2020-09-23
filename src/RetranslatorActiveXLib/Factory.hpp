#pragma once
#include <Unknwn.h>
#include "RetranslatorAX.hpp"

class Factory: public IClassFactory {
  public:
   Factory();
   ~Factory();
   HRESULT __stdcall QueryInterface(REFIID rrid, void **pv) override;
   ULONG __stdcall AddRef() override;
   ULONG __stdcall Release() override;

   HRESULT __stdcall CreateInstance(IUnknown *outer, const IID &iid, void **ppv) override;
   HRESULT __stdcall LockServer(BOOL lock);
  private:
   std::atomic_long m_refCount = 0;
};