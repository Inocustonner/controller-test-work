#include <windows.h>
#include <objbase.h>

#define OBJECT_ID "RetranslatorLib.RetranslatorAX"
#define OBJECT_DESCRIPTION "Interface for RetranslatorAX class"
#define OBJECT_MAJOR_VERSION "1"
#define OBJECT_MINOR_VERSION "0"
#define CLASS_DESCRIPTION "Retranslator ActiveX class"
#define INTERFACE "IRetranslator"
#define LIB_ID "{F5D56A47-4C01-4C07-A033-3C8B87787F71}"
#define IFCE_ID "{097344BD-7C6B-4F8C-9787-78C85E9EAC8B}"
#define CLS_ID "{6425EC19-B96A-4648-B24A-52997A34911E}"

extern HMODULE g_module;

const char *g_RegTable[][3] = {
	{ OBJECT_ID, 0, OBJECT_DESCRIPTION },
	{ OBJECT_ID "\\" OBJECT_DESCRIPTION, 0, OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION },
	{ OBJECT_ID "\\Clsid", 0, CLS_ID },

	{ OBJECT_ID "." OBJECT_MAJOR_VERSION, OBJECT_DESCRIPTION },
	{ OBJECT_ID ".1\\" OBJECT_DESCRIPTION, 0, OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION },
	{ OBJECT_ID ".1\\Clsid", 0, CLS_ID },
	{ OBJECT_ID ".1\\TypeLib", 0, CLS_ID },

	{ "Interface\\" IFCE_ID, 0, INTERFACE },
	{ "Interface\\" IFCE_ID "\\TypeLib", 0, LIB_ID },

	{ "CLSID\\" CLS_ID, 0, CLASS_DESCRIPTION },
	{ "CLSID\\" CLS_ID "\\ProgID", 0, OBJECT_ID "." OBJECT_MAJOR_VERSION },
	{ "CLSID\\" CLS_ID "\\VersionIndependentProgID", 0, OBJECT_ID },
	{ "CLSID\\" CLS_ID "\\InprocServer32", 0, (const char *)-1 },
	{ "CLSID\\" CLS_ID "\\InprocServer32", "ThreadingModel", "Both" },
	{ "CLSID\\" CLS_ID "\\TypeLib", 0, CLS_ID },

	/* Regestier the Type Info. */
	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION, 0, OBJECT_ID OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION " Library" },
	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\HELPDIR", 0, "" },
	{ "TypeLib\\" LIB_ID "\\" OBJECT_MAJOR_VERSION "." OBJECT_MINOR_VERSION "\\9\\win32", 0, (const char *)-1 },

	/* Interface registration. */
	{ "Interface\\" IFCE_ID, 0, INTERFACE },
	{ "Interface\\" IFCE_ID "\\TypeLib", 0, LIB_ID },
	/* IDispatch derived interface*/
	{ "Interface\\" IFCE_ID "\\ProxyStubClsid32", 0, "{00020400-0000-0000-C000-000000000046}" },

	/* Register as Safe for Scripting. */
	{ "Component\\Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
	{ "CLSID\\" CLS_ID "\\Implemented Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
	{ "Component\\Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
	{ "CLSID\\" CLS_ID "\\Implemented Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", 0, NULL },
};

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