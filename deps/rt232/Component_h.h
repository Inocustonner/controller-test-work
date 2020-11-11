

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 07:14:07 2038
 */
/* Compiler settings for Component.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __Component_h_h__
#define __Component_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRt232_FWD_DEFINED__
#define __IRt232_FWD_DEFINED__
typedef interface IRt232 IRt232;

#endif 	/* __IRt232_FWD_DEFINED__ */


#ifndef __Rt232_FWD_DEFINED__
#define __Rt232_FWD_DEFINED__

#ifdef __cplusplus
typedef class Rt232 Rt232;
#else
typedef struct Rt232 Rt232;
#endif /* __cplusplus */

#endif 	/* __Rt232_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __Component_LIBRARY_DEFINED__
#define __Component_LIBRARY_DEFINED__

/* library Component */
/* [helpstring][uuid] */ 


EXTERN_C const IID LIBID_Component;

#ifndef __IRt232_INTERFACE_DEFINED__
#define __IRt232_INTERFACE_DEFINED__

/* interface IRt232 */
/* [oleautomation][version][uuid][object] */ 


EXTERN_C const IID IID_IRt232;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B4686C7E-A759-4333-8EFF-4868110C980C")
    IRt232 : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE openPort( 
            /* [in] */ unsigned long port_n,
            /* [retval][out] */ long *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE closePort( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getErrorsCnt( 
            /* [retval][out] */ VARIANT *error_cnt) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IRt232Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRt232 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRt232 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRt232 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRt232 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRt232 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRt232 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRt232 * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *openPort )( 
            IRt232 * This,
            /* [in] */ unsigned long port_n,
            /* [retval][out] */ long *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *closePort )( 
            IRt232 * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getErrorsCnt )( 
            IRt232 * This,
            /* [retval][out] */ VARIANT *error_cnt);
        
        END_INTERFACE
    } IRt232Vtbl;

    interface IRt232
    {
        CONST_VTBL struct IRt232Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRt232_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IRt232_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IRt232_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IRt232_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IRt232_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IRt232_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IRt232_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IRt232_openPort(This,port_n,success)	\
    ( (This)->lpVtbl -> openPort(This,port_n,success) ) 

#define IRt232_closePort(This)	\
    ( (This)->lpVtbl -> closePort(This) ) 

#define IRt232_getErrorsCnt(This,error_cnt)	\
    ( (This)->lpVtbl -> getErrorsCnt(This,error_cnt) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IRt232_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Rt232;

#ifdef __cplusplus

class DECLSPEC_UUID("F5B223DF-B891-45B6-AA36-FDBC175753A2")
Rt232;
#endif
#endif /* __Component_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


