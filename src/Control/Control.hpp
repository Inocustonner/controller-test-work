#include <mutex>

enum class Cmd : int
{
	InitDb,
	// ExecQuery,
	// UpdQuery,

	Store_Store,
	Store_Store_Info,
	Store_Debug,
	Read_Cars,

	Done,
	Err,

	Exit
};


enum class DataType : int
{
	Str,
	Int,
	// Byte
};

#pragma pack(push, 1)
struct data_s
{
	int size;
	DataType type;

	inline
	void* body()
	{
		return this + 1;
	}
};


struct command_s
{
	Cmd cmd;
	//data_s* data_p;
};
#pragma pack(pop)

namespace Control
{
	using HANDLE = void*;
	HANDLE find_proc(const char* proc_name);
	HANDLE start_proc(const char* module_name);

	void InitShared();
	void OpenShared();
	void RemoveShared();

	command_s* get_command();
	data_s* next_data(data_s* data_p = nullptr);

	std::mutex& get_main_mutex();

	void CreateEventMain();
	void CreateEventDb();

	void CreateEventDebug();

	void OpenEventMain();
	void OpenEventDb();

	void OpenEventDebug();

	void CreateMutexStore();
	void CreateMutexDebug();	// for debug db


	void OpenMutexStore();
	void OpenMutexDebug();		// for debug db

	void SetEventMain();
	void SetEventDb();
	void SetEventDebug();

	void UnsetEventMain();
	void UnsetEventDb();
	void UnsetEventDebug();

	void releaseMutexStore();
	void releaseMutexDebug();

	void CloseEvents();

	void syncMain();
	void syncDb();
	void syncDebug();
	void lockMutexStore();
	void lockMutexDebug();
}
