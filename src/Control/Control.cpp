#include "Control.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstdio>
#include <windows.h>
#undef CreateEvent
#undef OpenEvent
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

static HANDLE event_main, event_db, event_debug, mutex_store;


#define TRYWIN(func, msg, ...) if (!func) { std::memset(error_buffer, '\0', std::size(error_buffer)); sprintf_s(error_buffer, std::size(error_buffer), msg, __VA_ARGS__); throw error_buffer; }

namespace Control
{
char error_buffer[1024];


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

	shared_memory = BoostSMem(boost::interprocess::open_only, memory_name, read_write);

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
	::ReleaseMutex(mutex_store);
}

inline
void OpenEvent(HANDLE& event, const wchar_t* ev_name)
{
	event = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE,
	FALSE,
	ev_name);

	if (event_main == NULL)
	{
		std::memset(error_buffer, '\0', std::size(error_buffer));
		sprintf(error_buffer, "Error openning %ls event: %d", ev_name, GetLastError());
		throw error_buffer;
		// get message box showing error
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

inline
void OpenMutex(HANDLE& handle, const wchar_t* name)
{
	TRYWIN((handle = ::OpenMutexW(SYNCHRONIZE, TRUE, name)), "%d: Failed to open %ls mutex\n", GetLastError(), name);
}


void OpenMutexStore()
{
	TRYWIN(mutex_store, mutex_store_wstr);
}


void CloseEvents()
{
	CloseHandle(event_main);
	CloseHandle(event_db);
	CloseHandle(event_debug);
	CloseHandle(mutex_store);
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
	UnsetEventDebug();
}


void lockMutexStore()
{
	WaitForSingleObject(mutex_store, INFINITE);
}
}
