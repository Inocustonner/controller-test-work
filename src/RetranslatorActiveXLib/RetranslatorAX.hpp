#pragma once
#include "include/IRetranslatorAX.hpp"
#define IS_EQ_IID(riid1, riid2) \
  ((IsEqualGUID(riid1, IID_IUnknown)) || (IsEqualGUID(riid1, riid2)))

#define IS_EQ_IID3(riid1, riid2, riid3) \
  ((IsEqualGUID(riid1, IID_IUnknown)) || (IsEqualGUID(riid1, riid2)) || (IsEqualGUID(riid1, riid3)))

class RetranslatorAX: public IRetranslatorAX {
  public:
   RetranslatorAX();
   ~RetranslatorAX();

   HRESULT __stdcall get_getWeight(long *res) override;
   HRESULT __stdcall get_getWeightFixed(long *res) override;

   // these 2 methods are not declared in .idl, because they are intended to be used by retranslator only
   void __stdcall setWeight(long weight) override;
   void __stdcall setWeightFixed(long weight) override;

   HRESULT __stdcall QueryInterface(REFIID riid, void **ppv) override;
   ULONG __stdcall AddRef() override;
   ULONG __stdcall Release() override;

   HRESULT __stdcall GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti) override;
   HRESULT __stdcall GetTypeInfoCount(UINT *pit) override;
   HRESULT __stdcall GetIDsOfNames(REFIID riid, OLECHAR **pNames, UINT cNames, LCID lcid, DISPID *dispids) override;
   HRESULT __stdcall Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pd, VARIANT *pVarResult, EXCEPINFO *pe, UINT *pu);
  private:
   std::atomic_long m_refCount;
   ITypeInfo *m_typeInfo;
};