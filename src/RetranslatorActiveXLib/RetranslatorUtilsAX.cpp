// for isInternetConnected
#include <winsock2.h>
#include <ws2tcpip.h>

#include "PipeQueueSystem/PipeQueueSystem.hpp"
#include "RetranslatorClassesAX.hpp"
#include <Retranslator_i.c>

#include <string>

#include <comutil.h>
#include <tlhelp32.h>
#include <windows.h>
#undef max


#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "comsuppwd.lib")

// for isInternetConnected
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define INET_CHECK_DOMAIN "www.google.com"

extern HMODULE g_module;
extern std::atomic_long g_objsInUse;

template<typename R>
bool is_future_ready(std::future<R> const& f)
{
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

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
  start_pipe_queue(logger_enabled, logger);

  // start internet check
  g_objsInUse++;
}

RetranslatorUtilsAX::~RetranslatorUtilsAX() {
  if (m_typeInfo) {
    m_typeInfo->Release();
  }
  stop_pipe_queue();

  g_objsInUse--;
}

void RetranslatorUtilsAX::log(const char* format, ...) {
  if (logger_enabled) {
    va_list args;
    va_start(args, format);

    int buf_size = vsnprintf(nullptr, 0, format, args);

    std::string line;
    line.resize(buf_size, 0);
    vsnprintf(line.data(), line.size() + 1, format, args);

    logger.log(line);
    va_end(args);
  }
}

HRESULT __stdcall RetranslatorUtilsAX::enableLogging(_In_ VARIANT* file_path) {
  logger_enabled = true;
  logger = SLogger(V_BSTR(file_path));
  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::start(VARIANT* cmd) {
  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi = {};
  BOOL succ = CreateProcessW(NULL, V_BSTR(cmd), NULL, NULL, FALSE,
                             NULL, NULL, NULL, &si, &pi);
  if (succ) {
    log("SUCCESS: Start command %ls", V_BSTR(cmd));
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);  
  }
  else {
    log("ERROR: Start command %ls", V_BSTR(cmd));
  }
  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::stop(VARIANT* exe_name) {
  long pid;
  log("Stopping %ls", V_BSTR(exe_name));
  getPID(exe_name, &pid);
  if (pid == -1) return S_OK;
  HWND hwnd = GetTopWindow(NULL);
  EnumWindows([](HWND hwnd, LPARAM pid) -> BOOL{
    DWORD wnd_pid;
    GetWindowThreadProcessId(hwnd, &wnd_pid);
    if (wnd_pid == pid) {
      SendMessage(hwnd, WM_CLOSE, NULL, NULL);
    }
    return TRUE;
  }, (LPARAM)pid);
  return S_OK;
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
  log("Set timeout %d ms", ms_timeout);
  pipe_set_timeout(ms_timeout);
  return S_OK;
}

long openService(LPWSTR service_name, SC_HANDLE* manager, SC_HANDLE* service) {
  *manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!*manager) {
    *service = nullptr;
    return GetLastError();
  }
  *service = OpenServiceW(*manager, service_name, SERVICE_ALL_ACCESS);
  if (!service) {
    CloseServiceHandle(*manager);
    *manager = nullptr;
    return GetLastError();
  }
  return 0;
}

#define INIT_OPEN_SERVICE \
  SC_HANDLE manager = NULL, service = NULL; \
  *status = openService(V_BSTR(serviceName), &manager, &service); \
  if (*status != 0) return S_OK

HRESULT __stdcall RetranslatorUtilsAX::startService(VARIANT* serviceName, _Out_  long* status) {
  INIT_OPEN_SERVICE;
  SERVICE_STATUS_PROCESS ssp;
  DWORD _;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &_)) {
    goto exit;
  }
  if (ssp.dwCurrentState == SERVICE_RUNNING || ssp.dwCurrentState == SERVICE_START_PENDING) {
    goto exit;
  }
  if (!ChangeServiceConfigW(
    service,        // handle of service 
    SERVICE_NO_CHANGE, // service type: no change 
    SERVICE_DEMAND_START,  // service start type 
    SERVICE_NO_CHANGE, // error control: no change 
    NULL,              // binary path: no change 
    NULL,              // load order group: no change 
    NULL,              // tag ID: no change 
    NULL,              // dependencies: no change 
    NULL,              // account name: no change 
    NULL,              // password: no change 
    NULL))            // display name: no change
  {
    goto exit;
  }
  StartServiceW(service, 0, NULL);
exit:
  *status = GetLastError();
  CloseServiceHandle(service);
  CloseServiceHandle(manager);
  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::stopService(VARIANT* serviceName, _Out_  long* status) {
  INIT_OPEN_SERVICE;
  SERVICE_STATUS_PROCESS ssp;
  DWORD _;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &_)) {
    goto exit;
  }
  if (ssp.dwCurrentState == SERVICE_STOP || ssp.dwCurrentState == SERVICE_STOP_PENDING) {
    goto exit;
  }
  if (!ChangeServiceConfigW(
    service,        // handle of service 
    SERVICE_NO_CHANGE, // service type: no change 
    SERVICE_DISABLED,  // service start type 
    SERVICE_NO_CHANGE, // error control: no change 
    NULL,              // binary path: no change 
    NULL,              // load order group: no change 
    NULL,              // tag ID: no change 
    NULL,              // dependencies: no change 
    NULL,              // account name: no change 
    NULL,              // password: no change 
    NULL))            // display name: no change
  {
    goto exit;
  }
  // assume there is no dependent services :)
  ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp);
exit:
  *status = GetLastError();
  CloseServiceHandle(service);
  CloseServiceHandle(manager);
  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::queryServiceStatus(VARIANT* serviceName, _Out_  long* status) {
  INIT_OPEN_SERVICE;
  SERVICE_STATUS_PROCESS ssp;
  DWORD _;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &_)) {
    *status = -(long)GetLastError();
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return S_OK;
  }
  else {
    *status = ssp.dwCurrentState;
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return S_OK;
  }
}

HRESULT __stdcall RetranslatorUtilsAX::getPID(VARIANT* proc_name, _Out_  long* pid) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32W pe = { sizeof(pe) };
  *pid = -1;

  if (!Process32FirstW(snapshot, &pe)) {
    CloseHandle(snapshot);
    return S_OK;
  }

  do {
    if (wcscmp(pe.szExeFile, V_BSTR(proc_name)) == 0) {
      *pid = pe.th32ProcessID;
      break;
    }
  } while (Process32NextW(snapshot, &pe));
  CloseHandle(snapshot);

  return S_OK;
}

HRESULT __stdcall RetranslatorUtilsAX::isInternetConnected(_Out_  long* Bool) {
  return E_NOTIMPL;
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
