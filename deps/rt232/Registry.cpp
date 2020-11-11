#include <windows.h>
#include <objbase.h>

#define OBJECT_MAJOR_VERSION "1"
#define OBJECT_MINOR_VERSION "0"
#define LIB_ID "{7429FA1E-842E-4872-BEEE-DB1982AA2239}"

// -----
//#define OBJECT_ID "Component.rt232"
#define OBJECT_ID "AddIn.rt232"
#define OBJECT_DESCRIPTION "Com 232 component"

#define CLASS_DESCRIPTION "1C component"
#define INTERFACE_NAME "IRt232"

#define IFCE_ID "{B4686C7E-A759-4333-8EFF-4868110C980C}"
#define CLS_ID "{F5B223DF-B891-45B6-AA36-FDBC175753A2}"

extern HMODULE g_module;

#define REGISTER_ACTIVEX_OBJECT(_OBJECT_ID, _OBJECT_DESCRIPTION, _CLS_GUID, _CLASS_DESCRIPTION, _INTERFACE, _INTERFACE_GUID, _LIB_GUID) \
	{ _OBJECT_ID, 0, _OBJECT_DESCRIPTION }, \
	{ _OBJECT_ID "\\" _OBJECT_DESCRIPTION, 0, OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION }, \
	{ _OBJECT_ID "\\CLSID", 0, _CLS_GUID }, \
	\
	{ _OBJECT_ID "." OBJECT_MAJOR_VERSION, _OBJECT_DESCRIPTION }, \
	{ _OBJECT_ID ".1\\" _OBJECT_DESCRIPTION, 0, OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION },\
	{ _OBJECT_ID ".1\\CLSID", 0, _CLS_GUID }, \
	/*{ _OBJECT_ID ".1\\TypeLib", 0, _CLS_GUID },*/ \
	\
	{ "Interface\\" _INTERFACE_GUID, 0, _INTERFACE }, \
	{ "Interface\\" _INTERFACE_GUID "\\TypeLib", 0, _LIB_GUID }, \
	\
	{ "CLSID\\" _CLS_GUID, 0, _CLASS_DESCRIPTION }, \
	{ "CLSID\\" _CLS_GUID "\\ProgID", 0, _OBJECT_ID "." OBJECT_MAJOR_VERSION }, \
	{ "CLSID\\" _CLS_GUID "\\VersionIndependentProgID", 0, _OBJECT_ID }, \
	{ "CLSID\\" _CLS_GUID "\\InprocServer32", 0, (const char *)-1 }, \
	{ "CLSID\\" _CLS_GUID "\\InprocServer32", "ThreadingModel", "Both" }, \
	{ "CLSID\\" _CLS_GUID "\\TypeLib", 0, _CLS_GUID }, \
	\
	/* Interface registration. */ \
	{ "Interface\\" _INTERFACE_GUID, 0, _INTERFACE }, \
	{ "Interface\\" _INTERFACE_GUID "\\TypeLib", 0, _LIB_GUID }, \
	/* IDispatch derived interface*/ \
	{ "Interface\\" _INTERFACE_GUID "\\ProxyStubClsid32", 0, "{00020400-0000-0000-C000-000000000046}" }, \
	\
	/* Register as Safe for Scripting. */ \
	{ "Component\\Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", 0, NULL }, \
	{ "CLSID\\" _CLS_GUID "\\Implemented Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", 0, NULL }, \
	{ "Component\\Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", 0, NULL }, \
	{ "CLSID\\" _CLS_GUID "\\Implemented Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", 0, NULL }, \
	\
	/* Regestier the Type Info. */ \
	{ "TypeLib\\" _LIB_GUID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION, 0, _OBJECT_ID OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION " Library" }


const char* g_RegTable[][3] = {
	REGISTER_ACTIVEX_OBJECT(OBJECT_ID,
		OBJECT_DESCRIPTION,
		CLS_ID,
		CLASS_DESCRIPTION,
		INTERFACE_NAME,
		IFCE_ID,
		LIB_ID),

	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\HELPDIR", 0, "" },
	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\9\\win32", 0, (const char*)-1 },
};

STDAPI DllUnregisterServer(void)
{
	HRESULT hr = S_OK;
	int nEntries = sizeof(g_RegTable) / sizeof(*g_RegTable);
	for (int i = nEntries - 1; i >= 0; --i) {
		const char* pszKeyName = g_RegTable[i][0];

		long err = RegDeleteKeyA(HKEY_CLASSES_ROOT, pszKeyName);
		if (err != ERROR_SUCCESS) {
			hr = S_FALSE;
		}
	}
	return hr;
}

STDAPI DllRegisterServer(void)
{
	HRESULT hr = S_OK;
	char szFileName[MAX_PATH];
	GetModuleFileNameA(g_module, szFileName, MAX_PATH);
	int nEntries = sizeof(g_RegTable) / sizeof(*g_RegTable);
	for (int i = 0; SUCCEEDED(hr) && i < nEntries; ++i) {
		const char* pszKeyName = g_RegTable[i][0];
		const char* pszValueName = g_RegTable[i][1];
		const char* pszValue = g_RegTable[i][2];

		/* -1 is a special marker which says use the DLL file location as
		 * the value for the key */
		if (pszValue == (const char*)-1) {
			pszValue = szFileName;
		}

		HKEY hkey;
		long err = RegCreateKeyA(HKEY_CLASSES_ROOT, pszKeyName, &hkey);
		if (err == ERROR_SUCCESS) {
			if (pszValue != NULL) {
				err = RegSetValueExA(hkey, pszValueName, 0, REG_SZ, (const BYTE*)pszValue, (strlen(pszValue) + 1));
			}
			RegCloseKey(hkey);
		}
		if (err != ERROR_SUCCESS) {
			DllUnregisterServer();
			hr = E_FAIL;
		}
	}
	return hr;
}
