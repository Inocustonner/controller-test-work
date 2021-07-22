#pragma once

#include <Retranslator_i.h>
#include <atomic>
#include <future>
#include "SLogger.hpp"

#define IS_EQ_IID(riid1, riid2)                                                \
  ((IsEqualGUID(riid1, IID_IUnknown)) || (IsEqualGUID(riid1, riid2)))

#define IS_EQ_IID3(riid1, riid2, riid3)                                        \
  ((IsEqualGUID(riid1, IID_IUnknown)) || (IsEqualGUID(riid1, riid2)) ||        \
   (IsEqualGUID(riid1, riid3)))

class RetranslatorAX : public IRetranslator {
public:
  RetranslatorAX();
  ~RetranslatorAX();
  HRESULT __stdcall getWeight(long *res) override;
  HRESULT __stdcall getWeightFixed(long *res) override;
  HRESULT __stdcall getMinimalWeight(long *res) override;
  HRESULT __stdcall getCorr(long *res) override;

  HRESULT __stdcall setMaximalWeight(_In_ long val) override;
  HRESULT __stdcall setMinimalWeight(long val) override;
  HRESULT __stdcall setCorr(long val) override;
  HRESULT __stdcall setNull() override;

  HRESULT __stdcall getStatus(long *err_status) override;
  HRESULT __stdcall getAuth(long *f_auth) override;
  HRESULT __stdcall getStab(long *f_stability) override;

  HRESULT __stdcall setResetThr(double* koef) override;
  HRESULT __stdcall clearAuth() override;
  // these 2 methods are not declared in .idl, because they are intended to be
  // used by retranslator only void __stdcall setWeight(long weight) override;
  // void __stdcall setWeightFixed(long weight) override;

  HRESULT __stdcall QueryInterface(REFIID riid, void **ppv) override;
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  HRESULT __stdcall GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti) override;
  HRESULT __stdcall GetTypeInfoCount(UINT *pit) override;
  HRESULT __stdcall GetIDsOfNames(REFIID riid, OLECHAR **pNames, UINT cNames,
                                  LCID lcid, DISPID *dispids) override;
  HRESULT __stdcall Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags,
                           DISPPARAMS *pd, VARIANT *pVarResult, EXCEPINFO *pe,
                           UINT *pu) override;

private:
  std::atomic_long m_refCount;
  ITypeInfo *m_typeInfo;
};

class RetranslatorUtilsAX : public IRetranslatorUtils {
public:
  RetranslatorUtilsAX();
  ~RetranslatorUtilsAX();

  HRESULT __stdcall QueryInterface(REFIID riid, void **ppv) override;
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  void log(const char* format, ...);

  HRESULT __stdcall enableLogging(_In_ VARIANT* file_path) override;

  HRESULT __stdcall start(VARIANT* cmd) override;
  HRESULT __stdcall stop(VARIANT* exe_name) override;

  HRESULT __stdcall run(unsigned char *cmd) override;
  HRESULT __stdcall runW(VARIANT* cmd) override;
  HRESULT __stdcall setTimeout(unsigned long ms_timeout) override;

  HRESULT __stdcall startService(VARIANT* serviceName, _Out_  long* status) override;
  HRESULT __stdcall stopService(VARIANT* serviceName, _Out_  long* status) override;
  HRESULT __stdcall queryServiceStatus(VARIANT* serviceName, _Out_  long* status) override;

  HRESULT __stdcall getPID(VARIANT* proc_name, _Out_  long* pid) override;
  HRESULT __stdcall isInternetConnected(_Out_  long* Bool) override;

  HRESULT __stdcall GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti) override;
  HRESULT __stdcall GetTypeInfoCount(UINT *pit) override;
  HRESULT __stdcall GetIDsOfNames(REFIID riid, OLECHAR **pNames, UINT cNames,
                                  LCID lcid, DISPID *dispids) override;
  HRESULT __stdcall Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags,
                           DISPPARAMS *pd, VARIANT *pVarResult, EXCEPINFO *pe,
                           UINT *pu) override;

private:
  std::atomic_long m_refCount;
  ITypeInfo *m_typeInfo;
  bool logger_enabled = false;
  SLogger logger;
};
//
//class RetranslatorComPort : public IRetranslatorComPort {
//public:
//  RetranslatorComPort();
//  ~RetranslatorComPort();
//
//  HRESULT __stdcall openPort(unsigned long port_n, unsigned long baudrate,
//                             unsigned long bytesize, long *status) override;
//  HRESULT __stdcall closePort() override;
//
//  HRESULT __stdcall setSoftwareSuffix(byte suffix) override;
//  HRESULT __stdcall setReadTimeout(long ms, long *status) override;
//
//  HRESULT __stdcall write(const byte *info) override;
//  HRESULT __stdcall read(unsigned long read_size, BSTR* read_info) override;
//
//  HRESULT __stdcall getLastError(long *status) override;
//
//  HRESULT __stdcall QueryInterface(REFIID riid, void **ppv) override;
//  ULONG __stdcall AddRef() override;
//  ULONG __stdcall Release() override;
//
//  HRESULT __stdcall GetTypeInfo(UINT it, LCID lcid, ITypeInfo **ppti) override;
//  HRESULT __stdcall GetTypeInfoCount(UINT *pit) override;
//  HRESULT __stdcall GetIDsOfNames(REFIID riid, OLECHAR **pNames, UINT cNames,
//                                  LCID lcid, DISPID *dispids) override;
//  HRESULT __stdcall Invoke(DISPID id, REFIID riid, LCID lcid, WORD wFlags,
//                           DISPPARAMS *pd, VARIANT *pVarResult, EXCEPINFO *pe,
//                           UINT *pu) override;
//
//private:
//  std::atomic_long m_refCount;
//  ITypeInfo *m_typeInfo;
//  
//  HANDLE h_com = NULL;
//  BYTE m_suffix = 0xFF; // no suffix
//  WCHAR* port_name_static = NULL;
//};


