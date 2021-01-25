// #pragma comment(lib, "OneCore.lib")

#include "Component.rt232.hpp"
#include "Component_i.c"
#include "addin_i.c"

#include "include/magic_enum.hpp"
#include <optional>

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>

#define RT232_COM_METHOD COM_METHOD Rt232::
#define TYPELIB_NAME_WCHAR L"rt232.tlb"
#define DEFAULT_READ_TIMEOUT 1000

extern HMODULE g_module;

extern std::atomic_long g_objsInUse;

enum Rt232Props {};

enum Rt232Methods {};

constexpr char eof_char = '\r'; // '\n'?

static IAsyncEvent *m_async_interface;
static IUnknown *m_connection;
void Rt232::create_external_event(std::wstring &buffer) {
  if (m_async_interface) {

    wchar_t com_n_buffer[10] = {};
    wsprintf(com_n_buffer, L"%d", m_port_n);

    BSTR extension_name = SysAllocString(EXTENSION_NAME);
    BSTR message = SysAllocString(com_n_buffer);
    BSTR data = SysAllocString(buffer.c_str());
    m_async_interface->ExternalEvent(extension_name, message, data);
  }
}

DWORD __stdcall Rt232::run_reader(Rt232 *this_) {
  std::wstring buffer;

  bool prev_stop_thread = this_->stop_thread;
  this_->log("Stop thread flag = %s", this_->stop_thread ? "TRUE" : "FALSE");
  while (!this_->stop_thread) {
    char c = 0;
    DWORD read = 0;
    if (!ReadFile(this_->h_com, &c, 1, &read, NULL)) {
      DWORD err = GetLastError();
      this_->log("Port error %u(GetLastError code)", err);
      DWORD clear_flags =
          CE_BREAK | CE_FRAME | CE_OVERRUN | CE_RXOVER | CE_RXPARITY;
      ClearCommError(this_->h_com, &clear_flags, NULL);
      if (err == 0x16 || err == 0x6) {
        CloseHandle(this_->h_com);
        this_->h_com = INVALID_HANDLE_VALUE;
        this_->log("reopenning port COM%d", this_->m_port_n);
        this_->ensure_open_port(false);
      }
      this_->read_open_errors_cnt += 1;
      continue;
    } else if (read == 1) {
      if (c == '\r') {
        this_->create_external_event(buffer);
        buffer.clear();
      } else
        buffer.push_back((wchar_t)c);
    }
    if (this_->stop_thread != prev_stop_thread) {
      this_->log("Stop thread flag changed to %s", this_->stop_thread ? "TRUE" : "FALSE");
    }
    // read_open_errors_cnt = 0;
  }
  return 0;
}

bool Rt232::configure_port() {
  DCB dcb = {sizeof(DCB)};

  if (!GetCommState(h_com, &dcb))
    return false;
  dcb.BaudRate = CBR_9600;
  dcb.ByteSize = (BYTE)8;
  dcb.StopBits = ONESTOPBIT;

  dcb.EvtChar = '\r';
  // dcb.EofChar = '\n'; // doesn't work anyway

  if (!SetCommState(h_com, &dcb))
    return false;

  if (!SetCommMask(h_com, EV_RXFLAG))
    return false;

  COMMTIMEOUTS timeouts = {};
  if (!GetCommTimeouts(h_com, &timeouts))
    return false;

  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = DEFAULT_READ_TIMEOUT;
  if (!SetCommTimeouts(h_com, &timeouts))
    return false;

  return true;
}

bool Rt232::ensure_open_port(bool force, int max_try_cnt) {
  int i = 0;
  char comport_name[16] = {};
  sprintf_s(comport_name, "\\\\.\\COM%d", m_port_n);
  do {
    log("openning attempt #%d", i + 1);
    // h_com = OpenCommPort(m_port_n, GENERIC_WRITE | GENERIC_READ, NULL);
    h_com = CreateFileA(comport_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, 0, NULL);
    if (h_com != INVALID_HANDLE_VALUE) {
      if (configure_port()) {
        // read_open_errors_cnt = 0;
        return true;
      }
    }
    read_open_errors_cnt += 1;

    if (max_try_cnt == ++i)
      break;

    std::this_thread::sleep_for(std::chrono::seconds(1));
  } while (force);
  return false;
}

RT232_COM_METHOD openPort(unsigned long port_n, long *success) {
  // stop previous thread
  // closePort();

  m_port_n = port_n;
  log("Openning port COM%u", m_port_n);
  if (ensure_open_port(false)) {
    log("Port has been opened");
    *success = 0;
    stop_thread = false;
    // reader_thread = std::thread(&Rt232::run_reader, this);
    reader_thread =
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&Rt232::run_reader,
                     this, NULL, NULL);
    log("Starting thread id(%p)", reader_thread);
  } else {
    *success = 1;
  }
  return S_OK;
}

RT232_COM_METHOD closePort() {
  if (h_com != NULL) {
    log("Closing port COM%d", this->m_port_n);
    stop_thread = true;
    if (reader_thread != NULL && reader_thread != INVALID_HANDLE_VALUE) {
      constexpr DWORD wait_ms = 5000;
      log("Waiting for thread to stop id(%p)", reader_thread);
      if (WaitForSingleObject(reader_thread, wait_ms) == WAIT_TIMEOUT) {
        log("Terminating thread id(%p)", reader_thread);
        TerminateThread(reader_thread, 0);
      }
      CloseHandle(reader_thread);
      log("Thread has been stopped");
    }
  }
  if (h_com != NULL) {
    CloseHandle(h_com);
    log("Port has been closed");
    h_com = NULL;
  }
  return S_OK;
}

RT232_COM_METHOD getErrorsCnt(VARIANT *errors_cnt) {
  errors_cnt->vt = VT_I4;
  V_I4(errors_cnt) = read_open_errors_cnt;
  return S_OK;
}

RT232_COM_METHOD enableLogging(VARIANT* log_file_path) {
  logging = true;
  logger = SLogger{ V_BSTR(log_file_path) };
  return S_OK;
}

void Rt232::log(const char* format, ...) {
  if (logging) {
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

RT232_COM_METHOD Init(IDispatch *pConnection) {
  m_connection = (IUnknown *)pConnection;

  if (pConnection == nullptr)
    return E_FAIL;
  HRESULT hr;
  hr = m_connection->QueryInterface(IID_IAsyncEvent,
                                    (void **)&m_async_interface);
  if (FAILED(hr))
    return hr;

  hr = m_async_interface->SetEventBufferDepth(300);
  if (FAILED(hr))
    return hr;

  return S_OK;
}

RT232_COM_METHOD Done(void) { return S_OK; }

RT232_COM_METHOD GetInfo(SAFEARRAY **p_info) {
  VARIANT *var;
  assert((*p_info)->fFeatures == (FADF_VARIANT | FADF_HAVEVARTYPE));
  HRESULT hr = SafeArrayAccessData(*p_info, (void **)&var);
  if (FAILED(hr))
    return hr;
  var->vt = VT_I4;
  V_I4(var) = 2000;
  SafeArrayUnaccessData(*p_info);
  return S_OK;
}

RT232_COM_METHOD RegisterExtensionAs(BSTR *extension_name) {
  *extension_name = SysAllocString(EXTENSION_NAME);
  return S_OK;
}

RT232_COM_METHOD GetNProps(long *pProps) {
  *pProps = magic_enum::enum_count<Rt232Props>();
  return S_OK;
}

RT232_COM_METHOD FindProp(BSTR prop_name, long *p_prop_num) {
  return E_NOTIMPL;
}

RT232_COM_METHOD GetPropName(long prop_num, long prop_alias, BSTR *prop_name) {
  return E_NOTIMPL;
}

RT232_COM_METHOD GetPropVal(long prop_num, VARIANT *pvar_prop_val) {
  return E_NOTIMPL;
}

RT232_COM_METHOD SetPropVal(long prop_num, VARIANT *pvar_prop_val) {
  return E_NOTIMPL;
}

RT232_COM_METHOD IsPropReadable(long prop_num, BOOL *p_bool_readable) {
  return E_NOTIMPL;
}

RT232_COM_METHOD IsPropWritable(long prop_num, BOOL *p_bool_writable) {
  return E_NOTIMPL;
}

RT232_COM_METHOD GetNMethods(long *p_methods_cnt) {
  *p_methods_cnt = magic_enum::enum_count<Rt232Methods>();
  return S_OK;
}

RT232_COM_METHOD FindMethod(BSTR method_name, long *p_method_num) {
  return E_NOTIMPL;
}

RT232_COM_METHOD GetMethodName(long method_num, long method_alias,
                               BSTR *p_method_name) {
  return E_NOTIMPL;
}

RT232_COM_METHOD GetNParams(long method_num, long *p_params_cnt) {
  return E_NOTIMPL;
}

RT232_COM_METHOD GetParamDefValue(long method_num, long param_num,
                                  VARIANT *pvar_def_value) {
  return E_NOTIMPL;
}

RT232_COM_METHOD HasRetVal(long method_num, BOOL *has_ret_value) {
  return E_NOTIMPL;
}

RT232_COM_METHOD CallAsProc(long method_num, SAFEARRAY **params) {
  return E_NOTIMPL;
}

RT232_COM_METHOD CallAsFunc(long method_num, VARIANT *pvar_ret_val,
                            SAFEARRAY **params) {
  return E_NOTIMPL;
}

// IPropetyPage
RT232_COM_METHOD SetPageSite(IPropertyPageSite *page_site) { return E_NOTIMPL; }

RT232_COM_METHOD Activate(HWND parent, LPCRECT p_rect, BOOL modal) {
  return E_NOTIMPL;
}

RT232_COM_METHOD Deactivate(void) { return E_NOTIMPL; }

RT232_COM_METHOD GetPageInfo(PROPPAGEINFO *p_page_info) { return E_NOTIMPL; }

RT232_COM_METHOD SetObjects(ULONG cObjects, IUnknown **pp_unknwn) {
  return E_NOTIMPL;
}

RT232_COM_METHOD Show(UINT cmd_show) { return E_NOTIMPL; }

RT232_COM_METHOD Move(LPCRECT p_rect) { return E_NOTIMPL; }

RT232_COM_METHOD IsPageDirty(void) { return E_NOTIMPL; }

RT232_COM_METHOD Apply(void) { return E_NOTIMPL; }

RT232_COM_METHOD Help(LPCOLESTR help_dir) { return E_NOTIMPL; }

RT232_COM_METHOD TranslateAccelerator(MSG *p_msg) { return E_NOTIMPL; }

// IRunnableObject
RT232_COM_METHOD GetRunningClass(LPCLSID p_clsid) { return E_NOTIMPL; }

RT232_COM_METHOD Run(LPBINDCTX p_bc) { // calls at the start
  return S_OK;
}

BOOL __stdcall Rt232::IsRunning(void) { return TRUE; }

RT232_COM_METHOD LockRunning(BOOL lock, BOOL last_unlock_closes) {
  return E_NOTIMPL;
}

RT232_COM_METHOD SetContainedObject(BOOL conteined) { return E_NOTIMPL; }

Rt232::Rt232() {
  ITypeLib *typeLib = nullptr;

  OLECHAR fileName[MAX_PATH] = {};

  GetModuleFileNameW(g_module, fileName, std::size(fileName));
  auto tlb_path = std::wstring(fileName);
  tlb_path.erase(std::begin(tlb_path) + tlb_path.find_last_of(L'\\') + 1,
                 std::end(tlb_path));
  tlb_path += TYPELIB_NAME_WCHAR;

  HRESULT hr = LoadTypeLib(tlb_path.c_str(), &typeLib);
  if (SUCCEEDED(hr)) {
    hr = typeLib->GetTypeInfoOfGuid(IID_IRt232, &m_typeInfo);
    if (FAILED(hr))
      m_typeInfo = nullptr;
  }
  g_objsInUse++;
}

Rt232::~Rt232() {
  if (m_typeInfo)
    m_typeInfo->Release();
  if (m_async_interface)
    m_async_interface->Release();
  if (m_connection)
    m_connection->Release();
  closePort();
  g_objsInUse--;
}

COM_METHOD Rt232::QueryInterface(REFIID riid, void **ppv) {
#define EQ IsEqualGUID
  if (ppv == nullptr)
    return E_INVALIDARG;

  // if (EQ(riid, IID_IUnknown)) {
  //   this->AddRef();
  //   *ppv = static_cast<IUnknown *>(static_cast<IRt232 *>(this));
  //   return S_OK;

  // } else if (EQ(riid, IID_IInitDone)) {
  //   this->AddRef();
  //   *ppv = static_cast<IInitDone *>(this);
  //   return S_OK;

  // } else if (EQ(riid, IID_ILanguageExtender)) {
  //   this->AddRef();
  //   *ppv = static_cast<ILanguageExtender *>(this);
  //   return S_OK;

  // } else if (EQ(riid, IID_IPropertyPage)) {
  //   this->AddRef();
  //   *ppv = static_cast<IPropertyPage *>(this);
  //   return S_OK;

  // } else if (EQ(riid, IID_IRunnableObject)) {
  //   this->AddRef();
  //   *ppv = static_cast<IRunnableObject *>(this);
  //   return S_OK;
  // } else if (EQ(riid, IID_IDispatch)) {
  //   this->AddRef();
  //   *ppv = static_cast<IDispatch *>(this);
  //   return S_OK;
  // } else
  //   return E_NOINTERFACE;

  if (EQ(riid, IID_IUnknown)) {
    this->AddRef();
    *ppv = static_cast<IUnknown *>(static_cast<IRt232 *>(this));
    return S_OK;

  } else if (EQ(riid, IID_IInitDone)) {
    this->AddRef();
    *ppv = static_cast<IInitDone *>(this);
    return S_OK;

  } else if (EQ(riid, IID_ILanguageExtender)) {
    this->AddRef();
    *ppv = static_cast<ILanguageExtender *>(this);
    return S_OK;

  } else if (EQ(riid, IID_IPropertyPage)) {
    this->AddRef();
    *ppv = static_cast<IPropertyPage *>(this);
    return S_OK;

  } else if (EQ(riid, IID_IRunnableObject)) {
    this->AddRef();
    *ppv = static_cast<IRunnableObject *>(this);
    return S_OK;
  } else if (EQ(riid, IID_IDispatch)) {
    this->AddRef();
    *ppv = static_cast<IRt232 *>(this);
    return S_OK;
  } else
    return E_NOINTERFACE;

#undef EQ
}

ULONG __stdcall Rt232::AddRef() { return ++m_refCount; }

ULONG __stdcall Rt232::Release() {
  ULONG ref = --m_refCount;
  if (m_refCount == 0)
    delete this;
  return ref;
}

HRESULT __stdcall Rt232::GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti) {
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

HRESULT __stdcall Rt232::GetTypeInfoCount(UINT *pit) {
  if (!m_typeInfo || !pit)
    return E_NOTIMPL;
  *pit = 1;
  return S_OK;
}

HRESULT __stdcall Rt232::GetIDsOfNames(REFIID riid, OLECHAR **pNames,
                                       UINT cNames, LCID lcid,
                                       DISPID *pdispids) {
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispGetIDsOfNames(m_typeInfo, pNames, cNames, pdispids);
}

HRESULT __stdcall Rt232::Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pd, VARIANT *pVarResult,
                                EXCEPINFO *pe, UINT *pu) {
  if (!m_typeInfo)
    return E_NOTIMPL;
  return DispInvoke(this, m_typeInfo, id, wFlags, pd, pVarResult, pe, pu);
}
