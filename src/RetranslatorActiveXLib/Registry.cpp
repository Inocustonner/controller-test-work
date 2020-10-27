#include <windows.h>
#include <objbase.h>

#define OBJECT_MAJOR_VERSION "1"
#define OBJECT_MINOR_VERSION "0"
#define LIB_ID "{F5D56A47-4C01-4C07-A033-3C8B87787F71}"

// -----
#define OBJECT_ID "RetranslatorLib.RetranslatorAX"
#define OBJECT_DESCRIPTION "Interface for RetranslatorAX class"

#define CLASS_DESCRIPTION "Retranslator ActiveX class"
#define INTERFACE "IRetranslator"

#define IFCE_ID "{097344BD-7C6B-4F8C-9787-78C85E9EAC8B}"
#define CLS_ID "{6425EC19-B96A-4648-B24A-52997A34911E}"


// -----
#define OBJECT_UTILS_ID "RetranslatorLib.RetranslatorUtilsAX"
#define OBJECT_UTILS_DESCRIPTION "Interface for RetranslatorUtilsAX class"

#define CLASS_UTILS_DESCRIPTION "RetranslatorUtils class"
#define INTERFACE_UTILS "IRetranslatorUtils"

#define IFCE_UTILS_ID "{A5DAA4D9-6271-4DE3-9AA4-3232B839BFFD}"
#define CLS_UTILS_ID "{280F8E09-7C79-403C-A944-436C5A66EFB6}"


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
		
const char *g_RegTable[][3] = {
	// utils class
	REGISTER_ACTIVEX_OBJECT(OBJECT_UTILS_ID,
		OBJECT_UTILS_DESCRIPTION,
		CLS_UTILS_ID,
		CLASS_UTILS_DESCRIPTION,
		INTERFACE_UTILS,
		IFCE_UTILS_ID,
		LIB_ID),

	REGISTER_ACTIVEX_OBJECT(OBJECT_ID,
		OBJECT_DESCRIPTION,
		CLS_ID,
		CLASS_DESCRIPTION,
		INTERFACE,
		IFCE_ID,
		LIB_ID),

	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\HELPDIR", 0, "" },
	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\9\\win32", 0, (const char*)-1 },
};
		
// const char *g_RegTable[][3] = {
// 	{ OBJECT_ID, 0, OBJECT_DESCRIPTION },
// 	{ OBJECT_ID "\\" OBJECT_DESCRIPTION, 0, OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION },
// 	{ OBJECT_ID "\\Clsid", 0, CLS_ID },

// 	{ OBJECT_ID "." OBJECT_MAJOR_VERSION, OBJECT_DESCRIPTION },
// 	{ OBJECT_ID ".1\\" OBJECT_DESCRIPTION, 0, OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION },
// 	{ OBJECT_ID ".1\\Clsid", 0, CLS_ID },
// 	{ OBJECT_ID ".1\\TypeLib", 0, CLS_ID },

// 	{ "Interface\\" IFCE_ID, 0, INTERFACE },
// 	{ "Interface\\" IFCE_ID "\\TypeLib", 0, LIB_ID },

// 	{ "CLSID\\" CLS_ID, 0, CLASS_DESCRIPTION },
// 	{ "CLSID\\" CLS_ID "\\ProgID", 0, OBJECT_ID "." OBJECT_MAJOR_VERSION },
// 	{ "CLSID\\" CLS_ID "\\VersionIndependentProgID", 0, OBJECT_ID },
// 	{ "CLSID\\" CLS_ID "\\InprocServer32", 0, (const char *)-1 },
// 	{ "CLSID\\" CLS_ID "\\InprocServer32", "ThreadingModel", "Both" },
// 	{ "CLSID\\" CLS_ID "\\TypeLib", 0, CLS_ID },

// 	/* Regestier the Type Info. */
// 	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION, 0, OBJECT_ID OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION " Library" },
// 	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\HELPDIR", 0, "" },
// 	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\9\\win32", 0, (const char *)-1 },

// 	/* Interface registration. */
// 	{ "Interface\\" IFCE_ID, 0, INTERFACE },
// 	{ "Interface\\" IFCE_ID "\\TypeLib", 0, LIB_ID },
// 	/* IDispatch derived interface*/
// 	{ "Interface\\" IFCE_ID "\\ProxyStubClsid32", 0, "{00020400-0000-0000-C000-000000000046}" },

// 	/* Register as Safe for Scripting. */
// 	{ "Component\\Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
// 	{ "CLSID\\" CLS_ID "\\Implemented Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
// 	{ "Component\\Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
// 	{ "CLSID\\" CLS_ID "\\Implemented Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
// };

STDAPI DllUnregisterServer(void)
{
	HRESULT hr = S_OK;
	int nEntries = sizeof(g_RegTable)/sizeof(*g_RegTable);
	for (int i = nEntries - 1; i >= 0; --i) {
		const char *pszKeyName = g_RegTable[i][0];

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
	int nEntries = sizeof(g_RegTable)/sizeof(*g_RegTable);
	for (int i = 0; SUCCEEDED(hr) && i < nEntries; ++i) {
		const char *pszKeyName = g_RegTable[i][0];
		const char *pszValueName = g_RegTable[i][1];
		const char *pszValue = g_RegTable[i][2];

		/* -1 is a special marker which says use the DLL file location as
		 * the value for the key */
		if (pszValue == (const char *)-1) {
			pszValue = szFileName;
		}

		HKEY hkey;
		long err = RegCreateKeyA(HKEY_CLASSES_ROOT, pszKeyName, &hkey);
		if (err == ERROR_SUCCESS) {
			if (pszValue != NULL) {
				err = RegSetValueExA(hkey, pszValueName, 0, REG_SZ, (const BYTE *)pszValue, (strlen(pszValue) + 1));
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
