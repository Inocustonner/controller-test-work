#pragma once
#include "RetranslatorClassesAX.hpp"
#include <Unknwn.h>

class Factory : public IClassFactory {
public:
  Factory(CLSID clsid);
  ~Factory();
  HRESULT __stdcall QueryInterface(REFIID rrid, void **pv) override;
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  HRESULT __stdcall CreateInstance(IUnknown *outer, const IID &iid,
                                   void **ppv) override;
  HRESULT __stdcall LockServer(BOOL lock);

private:
	CLSID m_clsid;
  std::atomic_long m_refCount = 0;
};
