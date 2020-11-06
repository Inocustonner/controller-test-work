#include "RetranslatorClassesAX.hpp"
#include <Retranslator_i.c>
#include "Hook.hpp"
#include "include/RetranslatorDefs.hpp"
#include <comutil.h>
#include <windows.h>
#include <string>

#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "comsuppwd.lib")

#define NO_CORR_WEIGHT 0 // receiving 0 value of for g_minimalWeight, means we don't want to apply correction
#define NO_CORR_WEIGHT_VALUE 999999 // big value what weight will never reach

#define InterlockedRead(var) InterlockedExchangeAdd(&(var), 0)
#define EXTERN_SHARED extern "C" volatile

#define INIT_STATUS(status_var) \
  Status status_var = {}; \
  InterlockedReadStatus(&status_var, &g_status);


extern HMODULE g_module;
extern std::atomic_long g_objsInUse;

EXTERN_SHARED long g_weightRaw;
EXTERN_SHARED long g_weightFixed;
EXTERN_SHARED Status g_status;
EXTERN_SHARED long g_minWeight;
EXTERN_SHARED long g_corr;

EXTERN_SHARED double g_reset_thr;

RetranslatorAX::RetranslatorAX()
    : m_refCount(0),
      m_typeInfo(nullptr)
{
  ITypeLib *typeLib = nullptr;

  OLECHAR fileName[MAX_PATH] = {};

  GetModuleFileNameW(g_module, fileName, std::size(fileName));
  auto tlb_path = std::wstring(fileName);
  tlb_path.erase(std::begin(tlb_path) + tlb_path.find_last_of(L'\\') + 1, std::end(tlb_path));
  tlb_path += L"Retranslator.tlb";

  HRESULT hr = LoadTypeLib(tlb_path.c_str(), &typeLib);
  if (SUCCEEDED(hr))
  {
    hr = typeLib->GetTypeInfoOfGuid(IID_IRetranslator, &m_typeInfo);
    if (FAILED(hr))
      m_typeInfo = nullptr;
  }
  g_objsInUse++;
}

RetranslatorAX::~RetranslatorAX()
{
  if (m_typeInfo)
    m_typeInfo->Release();
  g_objsInUse--;
}

HRESULT __stdcall RetranslatorAX::getWeight(long *res)
{
  *res = InterlockedRead(g_weightRaw);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::getWeightFixed(long *res)
{
  *res = InterlockedRead(g_weightFixed);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::getMinimalWeight(long *res) {
  auto m_w = InterlockedRead(g_minWeight);
  if (m_w == NO_CORR_WEIGHT_VALUE)
    *res = 0;
  else
    *res = m_w;
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::getCorr(long *res) {
  *res = InterlockedRead(g_corr);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::setMinimalWeight(long val) {
  if (val == NO_CORR_WEIGHT)
    InterlockedExchange(&g_minWeight, NO_CORR_WEIGHT_VALUE);
  else
    InterlockedExchange(&g_minWeight, val);
  fireEvent(SetMinimalWeight);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::setCorr(long val) {
  InterlockedExchange(&g_corr, val);
  fireEvent(SetCorr);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::setNull() {
  fireEvent(SetNull);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::getStatus(long *res) {
  INIT_STATUS(s);
  *res = s.err;
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::getAuth(long *f_auth) {
  INIT_STATUS(s);
  if (s.err == 0) {
    *f_auth = s.auth;
  }
  else {
    *f_auth = -1;
  }
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::getStab(long *f_stability) {
  INIT_STATUS(s);
  if (s.err == 0) {
    *f_stability = s.stability;
  }
  else {
    *f_stability = -1;
  }
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::setResetThr(double *koef) {
  g_reset_thr = *koef;
  fireEvent(SetResetThr);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::clearAuth() {
  fireEvent(ClearAuth);
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::QueryInterface(REFIID riid, void **ppv)
{
  if (ppv == nullptr)
    return E_INVALIDARG;
  if (IS_EQ_IID3(riid,
                 IID_IDispatch,
                 IID_IRetranslator))
  {
    this->AddRef();
    *ppv = static_cast<void *>(this);
    return S_OK;
  }
  else
  {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG __stdcall RetranslatorAX::AddRef()
{
  return ++m_refCount;
}

ULONG __stdcall RetranslatorAX::Release()
{
  ULONG ref = --m_refCount;
  if (m_refCount == 0)
    delete this;
  return ref;
}

HRESULT __stdcall RetranslatorAX::GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti)
{
  if (!m_typeInfo)
    return E_NOTIMPL;
  if (!ppti)
    return E_INVALIDARG;
  if (it != 0)
    return DISP_E_BADINDEX;

  m_typeInfo->AddRef();
  *ppti = m_typeInfo;

  return S_OK;
}

HRESULT __stdcall RetranslatorAX::GetTypeInfoCount(UINT *pit)
{
  if (!m_typeInfo || !pit)
    return E_NOTIMPL;
  *pit = 1;
  return S_OK;
}

HRESULT __stdcall RetranslatorAX::GetIDsOfNames(REFIID riid, OLECHAR **pNames, UINT cNames, LCID lcid, DISPID *pdispids)
{
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispGetIDsOfNames(m_typeInfo, pNames, cNames, pdispids);
}

HRESULT __stdcall RetranslatorAX::Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pd, VARIANT *pVarResult, EXCEPINFO *pe, UINT *pu)
{
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispInvoke(this, m_typeInfo, id, wFlags, pd, pVarResult, pe, pu);
}
