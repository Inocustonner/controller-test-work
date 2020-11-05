#include "PipeQueueSystem/PipeQueueSystem.hpp"
#include "RetranslatorClassesAX.hpp"
#include <Retranslator_i.c>

#include <comutil.h>
#include <string>
#include <windows.h>

#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "comsuppwd.lib")

extern HMODULE g_module;
extern std::atomic_long g_objsInUse;

RetranslatorUtilsAX::RetranslatorUtilsAX()
    : m_refCount(0), m_typeInfo(nullptr) {
  ITypeLib *typeLib = nullptr;

  OLECHAR fileName[MAX_PATH] = {};

  GetModuleFileNameW(g_module, fileName, std::size(fileName));
  auto tlb_path = std::wstring(fileName);
  tlb_path.erase(std::begin(tlb_path) + tlb_path.find_last_of(L'\\') + 1,
                 std::end(tlb_path));
  tlb_path += L"Retranslator.tlb";

  HRESULT hr = LoadTypeLib(tlb_path.c_str(), &typeLib);
  if (SUCCEEDED(hr)) {
    hr = typeLib->GetTypeInfoOfGuid(IID_IRetranslatorUtils, &m_typeInfo);
    if (FAILED(hr))
      m_typeInfo = nullptr;
  }
  start_pipe_queue();
  g_objsInUse++;
}

RetranslatorUtilsAX::~RetranslatorUtilsAX() {
  if (m_typeInfo) {
    m_typeInfo->Release();
  }
  stop_pipe_queue();
  g_objsInUse--;
}

HRESULT __stdcall RetranslatorUtilsAX::run(unsigned char *cmd) {
  size_t size = strlen(reinterpret_cast<char *>(cmd)) + 1;
  wchar_t *wchars = new (std::nothrow) wchar_t[size];
  if (wchars == nullptr)
    return E_OUTOFMEMORY;

  memset(wchars, 0, size * sizeof(WCHAR));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char *>(cmd), -1,
                      wchars, size - 1);

  pipe_push_cmd(wchars);

  delete[] wchars;

  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::runW(VARIANT* cmd) {
  pipe_push_cmd(V_BSTR(cmd));
  return S_OK;
}


HRESULT __stdcall RetranslatorUtilsAX::setTimeout(unsigned long ms_timeout) {
  pipe_set_timeout(ms_timeout);
  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::QueryInterface(REFIID riid, void **ppv) {
  if (ppv == nullptr)
    return E_INVALIDARG;
  if (IS_EQ_IID3(riid, IID_IDispatch, IID_IRetranslatorUtils)) {
    this->AddRef();
    *ppv = static_cast<void *>(this);
    return S_OK;
  } else {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG __stdcall RetranslatorUtilsAX::AddRef() { return ++m_refCount; }

ULONG __stdcall RetranslatorUtilsAX::Release() {
  ULONG ref = --m_refCount;
  if (m_refCount == 0)
    delete this;
  return ref;
}

HRESULT __stdcall RetranslatorUtilsAX::GetTypeInfo(UINT it, LCID lcid,
                                                   ITypeInfo **ppti) {
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

HRESULT __stdcall RetranslatorUtilsAX::GetTypeInfoCount(UINT *pit) {
  if (!m_typeInfo || !pit)
    return E_NOTIMPL;
  *pit = 1;
  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::GetIDsOfNames(REFIID riid,
                                                     OLECHAR **pNames,
                                                     UINT cNames, LCID lcid,
                                                     DISPID *pdispids) {
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispGetIDsOfNames(m_typeInfo, pNames, cNames, pdispids);
}

HRESULT __stdcall RetranslatorUtilsAX::Invoke(DISPID id, REFIID riid, LCID lcid,
                                              WORD wFlags, DISPPARAMS *pd,
                                              VARIANT *pVarResult,
                                              EXCEPINFO *pe, UINT *pu) {
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispInvoke(this, m_typeInfo, id, wFlags, pd, pVarResult, pe, pu);
}
