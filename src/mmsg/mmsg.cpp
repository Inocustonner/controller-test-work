#include <cstdlib>
#include <string>
#include <sstream>
#include <filesystem>
#include <thread>
#include <vector>
#define UNICODE
#include <Windows.h>
#include "SQueue.hpp"

namespace fs = std::filesystem;

static std::vector<HANDLE> h_vec; // sequencial memory for handles. The first handle is event handle, that will be fired on the new supply of handles.
static HANDLE event_h;			  // basicaly first handle in the vector
static SQueue<HANDLE> reciver;	  // thread safe queue from what handle will be recived.
static std::thread manager;		  // thread that will be run in runner_proc


static void runner_proc()
{
	constexpr int event_id = 0;
	h_vec.push_back(CreateEvent(NULL, TRUE, FALSE, TEXT("Read queue"))); // self resetable event.
	event_h = h_vec[0];
	if (event_h == NULL)
	{
		printf("%d: Failed to create event\n", GetLastError());
		return;
	}

	do
	{
		int rc = WaitForMultipleObjects(std::size(h_vec), h_vec.data(), FALSE, INFINITE);
		if (rc == WAIT_FAILED)
		{
			printf("%d: Wait failed on WaitForMultipleObjects\n", GetLastError());
			return;
		}
		else if (rc >= WAIT_ABANDONED_0)
		{
			printf("Error in objects managment\n");
		}
		else
		{
			if (rc == event_id)
			{	
				while (std::size(reciver))
				{
					printf("pushing msgbox handle %zu\n", h_vec.capacity());
					h_vec.push_back(reciver.pop());		
				}
			}
			else
			{
				printf("Closing/removing msgbox handle %zu\n", h_vec.capacity());
				if (!CloseHandle(h_vec[rc]))
					printf("%d: Error closing handle\n", GetLastError());
				h_vec.erase(std::begin(h_vec) + rc);
			}
		}
		ResetEvent(event_h);
	}while(true);
}


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
	
	reciver.push(pi.hThread);
	reciver.push(pi.hProcess);
	SetEvent(event_h);
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


void init_interface()
{
	manager = std::thread { runner_proc };
	manager.detach();
}
