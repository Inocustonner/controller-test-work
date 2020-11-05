#include "RetranslatorClassesAX.hpp"
#include <Retranslator_i.c>

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include <string>
#include <thread>

#include <comutil.h>
#include <windows.h>


#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "comsuppwd.lib")

// 1 sec
#define DEFAULT_READ_TIMEOUT 1000

extern HMODULE g_module;
extern std::atomic_long g_objsInUse;

void ensure_open_port(PHANDLE h_com, WCHAR *port_name) {
  *h_com = INVALID_HANDLE_VALUE;
  while (*h_com == INVALID_HANDLE_VALUE) {
    *h_com = CreateFileW(
      port_name,
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      OPEN_EXISTING,
      0,
      NULL
    );
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

BOOL setPortReadTimeout(HANDLE h_com, ULONG ms_timeout) {
  COMMTIMEOUTS timeouts = {};
  if (!GetCommTimeouts(h_com, &timeouts)) return FALSE;

  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = DEFAULT_READ_TIMEOUT;
  if (!SetCommTimeouts(h_com, &timeouts)) return FALSE;
  return TRUE;
}

BOOL configure_port(PHANDLE h_com, ULONG baudrate, ULONG bytesize) {
  DCB dcb = { sizeof(DCB) };

  if (!GetCommState(*h_com, &dcb)) return FALSE;
  dcb.BaudRate = baudrate;
  dcb.ByteSize = (BYTE)bytesize;
  
  if (!SetCommState(*h_com, &dcb)) return FALSE;

  if (!setPortReadTimeout(*h_com, DEFAULT_READ_TIMEOUT)) return FALSE;

  return TRUE;
}
#pragma warning (push)
#pragma warning (disable: 4996)
WCHAR* static_port_name(ULONG port_n) {
  constexpr auto port_name_buf_size = 20;
  static WCHAR port_name_buf[port_name_buf_size];
  
  ZeroMemory(port_name_buf, port_name_buf_size * sizeof(WCHAR));
  swprintf(port_name_buf, L"\\\\.\\COM%d", port_n);
  
  return port_name_buf;
}
#pragma warning (pop)

void ensure_open_and_configure_port(PHANDLE h_com, ULONG port_n, ULONG baudrate, ULONG bytesize) {

  BOOL succ = FALSE;
  WCHAR* port_name_buf = static_port_name(port_n);

    
  while (true) {
    ensure_open_port(h_com, port_name_buf);
    succ = configure_port(h_com, baudrate, bytesize);
    if (succ) break;
    else if (*h_com != INVALID_HANDLE_VALUE) CloseHandle(h_com);
  }
}

size_t ensure_com_read(PHANDLE ph_com, WCHAR* port_name_for_reopenning, ULONG to_read, byte* buffer) {
  size_t bytes_read = 0;
  while (bytes_read < to_read) {
    DWORD read_now = 0;
    BOOL succ = ReadFile(
      *ph_com,
      buffer + bytes_read,
      to_read - bytes_read,
      &read_now,
      NULL
    );
    if (!succ) {
      DWORD err = GetLastError();
      
      DWORD clear_flags = CE_BREAK | CE_FRAME | CE_OVERRUN | CE_RXOVER | CE_RXPARITY;
      ClearCommError(*ph_com, &clear_flags, NULL);
      
      if (err == 0x16) {
        CloseHandle(*ph_com);
        ensure_open_port(ph_com, port_name_for_reopenning);
        continue;
      }
    }

    if (read_now > 0){
      bytes_read += read_now;
    }
  }
  return bytes_read;
}

RetranslatorComPort::RetranslatorComPort()
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
    hr = typeLib->GetTypeInfoOfGuid(IID_IRetranslatorComPort, &m_typeInfo);
    if (FAILED(hr))
      m_typeInfo = nullptr;
  }
  g_objsInUse++;
}

RetranslatorComPort::~RetranslatorComPort() {
  if (m_typeInfo) {
    m_typeInfo->Release();
  }
  g_objsInUse--;
  closePort();
}

HRESULT __stdcall RetranslatorComPort::openPort(unsigned long port_n, unsigned long baudrate, unsigned long bytesize, long *status) {
  if (h_com != NULL ){
    closePort();
  }
  port_name_static = static_port_name(port_n);
  ensure_open_and_configure_port(&h_com, port_n, baudrate, bytesize);
  *status = 0;
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::closePort() {
  CloseHandle(h_com);
  h_com = NULL;
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::setSoftwareSuffix(byte suffix) {
  m_suffix = suffix;
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::setReadTimeout(long ms, long *status) {
  if (!setPortReadTimeout(h_com, ms)) *status = 1;
  else *status = 0;
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::write(const byte* info) {
  // com_write(h_com, info);
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::read(unsigned long read_size, BSTR* read_info) {
  if (!read_info) {
    return E_INVALIDARG;
  }
  byte *buffer = new (std::nothrow) byte[read_size];
  if (buffer == nullptr) {
    return E_OUTOFMEMORY;
  }
  
  ZeroMemory(buffer, read_size);
  
  size_t read_bytes = ensure_com_read(&h_com, port_name_static, read_size, buffer);
  if (read_bytes == 0) {
    delete[] buffer;
    
    return S_OK;
  }
  *read_info = new (std::nothrow) OLECHAR[read_bytes + 1];
  if (read_info == nullptr) {
      delete[] buffer;
      return E_OUTOFMEMORY;
  }
  ZeroMemory(*read_info, (read_bytes + 1) * sizeof(WCHAR));

  mbstowcs(*read_info, (char*)buffer, read_bytes);
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::getLastError(long *status) {
  *status = ::GetLastError();
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::QueryInterface(REFIID riid, void **ppv) {
  if (ppv == nullptr)
    return E_INVALIDARG;
  if (IS_EQ_IID3(riid, IID_IDispatch, IID_IRetranslatorComPort)) {
    this->AddRef();
    *ppv = static_cast<void *>(this);
    return S_OK;
  } else {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG __stdcall RetranslatorComPort::AddRef() { return ++m_refCount; }

ULONG __stdcall RetranslatorComPort::Release() {
  ULONG ref = --m_refCount;
  if (m_refCount == 0)
    delete this;
  return ref;
}

HRESULT __stdcall RetranslatorComPort::GetTypeInfo(UINT it, LCID lcid,
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

HRESULT __stdcall RetranslatorComPort::GetTypeInfoCount(UINT *pit) {
  if (!m_typeInfo || !pit)
    return E_NOTIMPL;
  *pit = 1;
  return S_OK;
}

HRESULT __stdcall RetranslatorComPort::GetIDsOfNames(REFIID riid,
                                                     OLECHAR **pNames,
                                                     UINT cNames, LCID lcid,
                                                     DISPID *pdispids) {
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispGetIDsOfNames(m_typeInfo, pNames, cNames, pdispids);
}

HRESULT __stdcall RetranslatorComPort::Invoke(DISPID id, REFIID riid, LCID lcid,
                                              WORD wFlags, DISPPARAMS *pd,
                                              VARIANT *pVarResult,
                                              EXCEPINFO *pe, UINT *pu) {
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispInvoke(this, m_typeInfo, id, wFlags, pd, pVarResult, pe, pu);
}
