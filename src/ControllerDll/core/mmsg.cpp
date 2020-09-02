#include <cstdlib>
#include <string>
#include <sstream>
#include <filesystem>
#include <vector>
#define UNICODE
#include <Windows.h>

namespace fs = std::filesystem;


static void run_proc(const wchar_t *exe, wchar_t *arg_line)
{
	STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    // Start the child process. 
    if(!CreateProcessW(exe,   // No module name (use command line)
		(LPWSTR)arg_line,
		NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
    )
    {
        printf("%d: CreateProcess failed\n", GetLastError());
        return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}


void mMsgBox(const wchar_t *body, const wchar_t *title, unsigned int msElapse)
{
	static const wchar_t* exe_name = L"msgb.exe";
	wchar_t buffer[1024] = {};
	GetModuleFileNameW(NULL, buffer, sizeof(buffer) / sizeof(TCHAR));

	std::wstringstream wsstr_args;
	std::wstring wstr_exe(buffer);
	wstr_exe.replace(std::cbegin(wstr_exe) + wstr_exe.find_last_of('\\') + 1, std::cend(wstr_exe), exe_name);
	wsstr_args << title << " " << body << " " << msElapse;
	run_proc(wstr_exe.c_str(), const_cast<wchar_t*>(wsstr_args.str().c_str()));
}
