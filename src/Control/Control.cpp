#include "Control.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstdio>
#include <windows.h>
#include <TLHelp32.h>

#undef CreateEvent
#undef OpenEvent
#undef OpenMutex
#undef CreateMutex

using BoostSMem = boost::interprocess::shared_memory_object;
using boost::interprocess::mapped_region;

static const char* memory_name = "ControllerMem";
constexpr int shared_size = 4096;
static BoostSMem shared_memory;

static mapped_region mapped_memory;

static command_s* command_p = nullptr;

static const wchar_t* event_main_wstr = L"EV1MAIN";
static const wchar_t* event_db_wstr = L"EV1DB";
static const wchar_t* event_debug_wstr = L"EV2DEBUG";
static const wchar_t* mutex_store_wstr = L"MUT5TOR3";
static const wchar_t* mutex_debug_wstr = L"MUTD3BUG";

static HANDLE event_main, event_db, event_debug, mutex_store, mutex_debug;


#define TRYWIN(func, msg, ...) if (!func) { std::memset(error_buffer, '\0', std::size(error_buffer)); sprintf_s(error_buffer, std::size(error_buffer), msg, __VA_ARGS__); throw std::exception(error_buffer); }

static std::mutex main_mutex;

namespace Control
{
char error_buffer[1024];


DWORD findProcessId(const char* proc_name)
{
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE)
	{
        return 0;
    }

    Process32First(processesSnapshot, &processInfo);
	do
	{
		if (!_stricmp(proc_name, processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	} while (Process32Next(processesSnapshot, &processInfo));

    CloseHandle(processesSnapshot);
    return 0;
}


HANDLE find_proc(const char* proc_name)
{
	if (DWORD pid = findProcessId(proc_name); pid != 0)
	{
		HANDLE ph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (ph != INVALID_HANDLE_VALUE)
			return ph;
	}
	return INVALID_HANDLE_VALUE;
}


HANDLE start_proc(const char* module_name)
{
	static char error_buffer[1024];

	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	if (!CreateProcessA(
		nullptr,
		(LPSTR)module_name,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&si,
		&pi))
	{
		fprintf(stderr, "CreateProcess error : %d\n", GetLastError());
		std::memset(error_buffer, '\0', std::size(error_buffer));
		sprintf(error_buffer, "CreateProcess error : %d", GetLastError());

		throw error_buffer;
	}
	CloseHandle(pi.hThread);	// bcs i dont use it

	return pi.hProcess;
}

void InitShared()
{
	using namespace boost::interprocess;
	BoostSMem::remove(memory_name);

	shared_memory = BoostSMem(boost::interprocess::create_only, memory_name, read_write);

	shared_memory.truncate(shared_size);
	mapped_memory = mapped_region(shared_memory, read_write);
	command_p = reinterpret_cast<command_s*>(mapped_memory.get_address());
}


void OpenShared()
{
	using namespace boost::interprocess;

	shared_memory = BoostSMem(boost::interprocess::open_or_create, memory_name, read_write);

	shared_memory.truncate(shared_size);
	mapped_memory = mapped_region(shared_memory, read_write);
	command_p = reinterpret_cast<command_s*>(mapped_memory.get_address());
}


void RemoveShared()
{
	BoostSMem::remove(memory_name);
}


command_s* get_command()
{
	return command_p;
}


data_s* next_data(data_s* data_p)
{
	if (data_p)
		return reinterpret_cast<data_s*>(reinterpret_cast<char*>(data_p + 1) + data_p->size);
	else
		return reinterpret_cast<data_s*>(command_p + 1);// command_p->data_p;
}


std::mutex& get_main_mutex()
{
	return main_mutex;
}


inline
void CreateEvent(HANDLE& event, const wchar_t* ev_name)
{
	// event is not yet created
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	event = CreateEventW(
		&sa,
		TRUE,
		FALSE,
		ev_name);

	if (event == NULL)
	{
		std::memset(error_buffer, '\0', std::size(error_buffer));
		sprintf(error_buffer, "Error creating %ls event: %d", ev_name, GetLastError());
		throw error_buffer;
		// get message box showing error
	}
}


void CreateEventMain()
{
	CreateEvent(event_main, event_main_wstr);
}


void CreateEventDb()
{
	CreateEvent(event_db, event_db_wstr);
}


void CreateEventDebug()
{
	CreateEvent(event_debug, event_debug_wstr);
}

void SetEventMain()
{
	TRYWIN(::SetEvent(event_main), "Failed to set %s event: %d", "Main", GetLastError());
}

void SetEventDb()
{
	TRYWIN(::SetEvent(event_db), "Failed to set %s event: %d", "Db", GetLastError());
}

void SetEventDebug()
{
	TRYWIN(::SetEvent(event_debug), "Failed to set %s event: %d", "Debug", GetLastError());
}

void UnsetEventMain()
{
	TRYWIN(::ResetEvent(event_main), "Failed to reset %s event: %d", "Main", GetLastError());
}

void UnsetEventDb()
{
	TRYWIN(::ResetEvent(event_db), "Failed to reset %s event: %d", "Db", GetLastError());
}

void UnsetEventDebug()
{
	TRYWIN(::ResetEvent(event_debug), "Failed to reset %s event: %d", "Debug", GetLastError());
}


void releaseMutexStore()
{
	TRYWIN(::ReleaseMutex(mutex_store), "%d Failed to release mutex %s", GetLastError(), "Store");
}

void releaseMutexDebug()
{
	TRYWIN(::ReleaseMutex(mutex_debug), "%d Failed to release mutex Debug", GetLastError());
}

inline
void OpenEvent(HANDLE& event, const wchar_t* ev_name)
{
	event = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE,
	FALSE,
	ev_name);

	if (event == NULL)
	{
		CreateEvent(event, ev_name);
	}
}


void OpenEventMain()
{
	OpenEvent(event_main, event_main_wstr);
}


void OpenEventDb()
{
	OpenEvent(event_db, event_db_wstr);
}


void OpenEventDebug()
{
	OpenEvent(event_debug, event_debug_wstr);
}

inline
void CreateMutex(HANDLE& handle, const wchar_t* name)
{
	SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
	TRYWIN((handle = ::CreateMutexW(&sa, FALSE, name)),
		"%d: Failed to create %ls mutex\n", GetLastError(), name);
}


void CreateMutexStore()
{
	CreateMutex(mutex_store, mutex_store_wstr);
}


void CreateMutexDebug()
{
	CreateMutex(mutex_debug, mutex_debug_wstr);
}

inline
void OpenMutex(HANDLE& handle, const wchar_t* name)
{
	TRYWIN((handle = OpenMutexW(MUTEX_ALL_ACCESS, TRUE, name)), "%d: Failed to open %ls mutex\n", GetLastError(), name);
}


void OpenMutexStore()
{
	OpenMutex(mutex_store, mutex_store_wstr);
}


void OpenMutexDebug()
{
	OpenMutex(mutex_debug, mutex_debug_wstr);
}


void CloseEvents()
{
	CloseHandle(event_main);
	CloseHandle(event_db);
	CloseHandle(event_debug);
	CloseHandle(mutex_store);
	CloseHandle(mutex_debug);
}


inline
void sync(const HANDLE* event)
{
	constexpr DWORD nEvents = 1;
	WaitForMultipleObjects(nEvents, event, TRUE, INFINITE);
}


void syncMain()
{
	sync(&event_main);
	UnsetEventMain();
}


void syncDb()
{
	sync(&event_db);
	UnsetEventDb();
}

void syncDebug()
{
	sync(&event_debug);
	//UnsetEventDebug();
}


inline
void lockMutex(HANDLE& handle)
{
	switch (WaitForSingleObject(handle, INFINITE))
	{
		case WAIT_OBJECT_0:
			break;
		default:
			TRYWIN(0, "%d:Failed to obtain ownership of mutex\n", GetLastError());
	}
}


void lockMutexStore()
{
	lockMutex(mutex_store);
}


void lockMutexDebug()
{
	lockMutex(mutex_debug);
}
}
