#include <Control.hpp>

#include <tuple>
#include <vector>
#include <iostream>

#include <Windows.h>

std::tuple<HANDLE, HANDLE> start_proc(const char* module_name)
{
	static char error_buffer[1024];

	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	if (!CreateProcessA(
		module_name,
		nullptr,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&si,
		&pi))
	{
		std::cerr << "CreateProcess error : " << GetLastError() << '\n';
		std::memset(error_buffer, '\0', std::size(error_buffer));
		sprintf(error_buffer, "CreateProcess error : %d", GetLastError());

		throw error_buffer;
	}

	return { pi.hThread, pi.hProcess };
}

int main()
{
	std::vector<HANDLE> handles;
	std::vector<HANDLE> thread_hs;
	handles.reserve(4);
	try
	{
		Control::InitShared();

		Control::CreateEventMain();
		Control::CreateEventDb();
#ifdef __DEBUG__
		Control::CreateEventDebug();
#endif
		std::tuple tup = start_proc("ControllerFree_v.3.2.exe");
		thread_hs.push_back(std::get<0>(tup));
		handles.push_back(std::get<1>(tup));

		tup = start_proc("dbproc.exe");
		thread_hs.push_back(std::get<0>(tup));
		handles.push_back(std::get<1>(tup));
	}
	catch (std::exception& e)
	{	
		std::cerr << e.what() << '\n';
		system("pause");
	}
#ifdef __DEBUG__
	Control::SetEventDebug();
#endif
	WaitForMultipleObjects(std::size(thread_hs), thread_hs.data(), FALSE, INFINITE);

	TerminateThread(thread_hs[0], 0);
	CloseHandle(thread_hs[0]);
	CloseHandle(handles[0]);

	TerminateThread(thread_hs[1], 0);
	CloseHandle(thread_hs[1]);
	CloseHandle(handles[1]);
	//to resume thread if he is waiting for an event
	Control::SetEventMain();
	Control::SetEventDb();
	Control::SetEventDebug();

	Control::RemoveShared();
	Control::CloseEvents();
	return 0;
}