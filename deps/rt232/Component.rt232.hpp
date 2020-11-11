#pragma once
#include "Component_h.h"
#include "addin_h.h"

#include <string>
#include <thread>
#include <atomic>

#define COM_METHOD HRESULT __stdcall
#define COM_OVERRIDE(method_args) COM_METHOD method_args override

#define EXTENSION_NAME L"Component_RT232"

class Rt232 : public IRt232,
              public IInitDone,
              public ILanguageExtender,
              public IPropertyPage,
              public IRunnableObject {
public:
  Rt232();
  ~Rt232();
  
  void create_external_event(std::wstring& buffer);

  static DWORD __stdcall run_reader(Rt232* this_);

  bool configure_port();
  bool ensure_open_port(bool force = true, int max_try_cnt = 0);

  COM_OVERRIDE(openPort(unsigned long port_n, long *success));
  COM_OVERRIDE(closePort());

  COM_OVERRIDE(getErrorsCnt(VARIANT *errors_cnt));


  COM_OVERRIDE(Init(IDispatch *pConnection));
  COM_OVERRIDE(Done(void));
  COM_OVERRIDE(GetInfo(SAFEARRAY* *pInfo));
  
  COM_OVERRIDE(RegisterExtensionAs(BSTR *extension_name));

  COM_OVERRIDE(GetNProps(long *pProps));
  COM_OVERRIDE(FindProp(BSTR prop_name, long *p_prop_num));

  COM_OVERRIDE(GetPropName(long prop_num, long prop_alias, BSTR *prop_name));
  COM_OVERRIDE(GetPropVal(long prop_num, VARIANT *pvar_prop_val));
  COM_OVERRIDE(SetPropVal(long prop_num, VARIANT *pvar_prop_val));
  
  COM_OVERRIDE(IsPropReadable(long prop_num, BOOL *p_bool_readable));
  COM_OVERRIDE(IsPropWritable(long prop_num, BOOL *p_bool_writable));

  COM_OVERRIDE(GetNMethods(long *p_methods_cnt));

  COM_OVERRIDE(FindMethod(BSTR method_name, long *p_method_num));
  COM_OVERRIDE(GetMethodName(long method_num, long method_alias, BSTR *p_method_name));

  COM_OVERRIDE(GetNParams(long method_num, long *p_params_cnt));
  COM_OVERRIDE(GetParamDefValue(long method_num, long param_num, VARIANT *pvar_def_value));

  COM_OVERRIDE(HasRetVal(long method_num, BOOL *has_ret_value));

  COM_OVERRIDE(CallAsProc(long method_num, SAFEARRAY* *params));
  COM_OVERRIDE(CallAsFunc(long method_num, VARIANT *pvar_ret_val, SAFEARRAY* *params));
  
  // IPropetyPage
  COM_OVERRIDE(SetPageSite(IPropertyPageSite* page_site));
  COM_OVERRIDE(Activate(HWND parent, LPCRECT p_rect, BOOL modal));
  COM_OVERRIDE(Deactivate(void));
  COM_OVERRIDE(GetPageInfo(PROPPAGEINFO* p_page_info));
  COM_OVERRIDE(SetObjects(ULONG cObjects, IUnknown** pp_unknwn));
  COM_OVERRIDE(Show(UINT cmd_show));
  COM_OVERRIDE(Move(LPCRECT p_rect));
  COM_OVERRIDE(IsPageDirty(void));
  COM_OVERRIDE(Apply(void));
  COM_OVERRIDE(Help(LPCOLESTR help_dir));
  COM_OVERRIDE(TranslateAccelerator(MSG* p_msg));

  // IRunnableObject
  COM_OVERRIDE(GetRunningClass(LPCLSID p_clsid));
  COM_OVERRIDE(Run(LPBINDCTX p_bc));
  BOOL __stdcall IsRunning(void) override;
  COM_OVERRIDE(LockRunning(BOOL lock, BOOL last_unlock_closes));
  COM_OVERRIDE(SetContainedObject(BOOL conteined));
  
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  COM_METHOD QueryInterface(REFIID riid, void **ppv) override;
  
  HRESULT __stdcall GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti) override;
  HRESULT __stdcall GetTypeInfoCount(UINT *pit) override;
  HRESULT __stdcall GetIDsOfNames(REFIID riid, OLECHAR **pNames, UINT cNames,
                                  LCID lcid, DISPID *dispids) override;
  HRESULT __stdcall Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags,
                           DISPPARAMS *pd, VARIANT *pVarResult, EXCEPINFO *pe,
                           UINT *pu) override;
private:
  std::atomic_long m_refCount;
  ITypeInfo *m_typeInfo = nullptr;

  HANDLE h_com = NULL;
  unsigned long m_port_n;
  bool stop_thread;
  HANDLE reader_thread = NULL;

  unsigned long read_open_errors_cnt = 0;
};
