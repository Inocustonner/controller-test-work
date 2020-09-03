#include <Control.hpp>

#include <tuple>
#include <vector>
#include <iostream>

#include <Windows.h>
#include <tlhelp32.h>

#include <thread>
#include <chrono>

void HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

void ShowConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}

bool IsConsoleVisible()
{
	return (::IsWindowVisible(::GetConsoleWindow()) != FALSE);
}

// if one closes then return from function and terminate both
void watchdog(std::vector<HANDLE> phandles)
{
	while (true)
	{
		using namespace std::chrono_literals;

		for (HANDLE ph : phandles)
		{
			DWORD retcode;
			if (GetExitCodeProcess(ph, &retcode))
			{
				if (retcode != STILL_ACTIVE) // means one of processes is not running anymore
					return;
			}
			else
			{
				std::cout << "Error occured in calling GetExitCodeProcess: " << GetLastError() << '\n';
				std::this_thread::sleep_for(30s);
				return;
			}
		}
		std::this_thread::sleep_for(1s);
	}
}

static std::string get_module_dir() noexcept
{
	char path_buf[MAX_PATH + 1] = {};
	GetModuleFileNameA(NULL, reinterpret_cast<char*>(path_buf), std::size(path_buf) - 1);

	auto path = std::string(path_buf);
	path.erase(std::begin(path) + path.find_last_of('\\') + 1, std::end(path));
	return path;
}

int main(int argc, char *argv[])
{
	const char *leading_proc = "ControllerFree_v.3.2.exe";
	if (argc == 2) {
		leading_proc = argv[1];
	}

	std::vector<HANDLE> handles;
	handles.reserve(4);
	try
	{
		Control::OpenShared();

		Control::CreateEventMain();
		Control::UnsetEventMain();

		Control::CreateEventDb();
		// Control::OpenEventDebug();

		Control::CreateMutexStore();
		Control::CreateMutexDebug();
		
		Control::SetEventMain();

		HANDLE tup = Control::find_proc(leading_proc);
		handles.push_back(tup);
		if (tup == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "'find_proc(\"ControllerFree_v.3.2.exe\")' returned invalid handle: %d", GetLastError());
			goto end;
		}

		tup = Control::start_proc((get_module_dir() + "dbproc.exe").c_str());
		handles.push_back(tup);
		if (tup == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "'start_proc(\"dbproc.exe\")' returned invalid handle: %d", GetLastError());
			goto end;
		}
	}
	catch (std::exception& e)
	{	
		std::cerr << e.what() << '\n';
		system("pause");
		goto end;
	}

#if 0
	HideConsole();
#endif
	// Control::SetEventDebug();

	watchdog(handles);

end:
	Control::get_command()->cmd = Cmd::Exit;
	try
	{
		Control::SetEventMain();
		Control::SetEventDb();
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "%s\n", e.what());
#ifdef __DEBUG__
		system("pause");
#endif
	}

	TerminateProcess(handles[0], 0);
	if (std::size(handles) > 1)
		TerminateProcess(handles[1], 0);

	CloseHandle(handles[0]);
	if (std::size(handles) > 1)
		CloseHandle(handles[1]);

	try
	{
		Control::CloseEvents();
		Control::RemoveShared();
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "%s\n", e.what());
#ifdef __DEBUG__
		system("pause");
#endif
	}
	return 0;
}