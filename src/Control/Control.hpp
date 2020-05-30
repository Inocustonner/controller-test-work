enum class Cmd : int
{
	InitDb,
	// ExecQuery,
	// UpdQuery,

	Store_Store,
	Store_Store_Info,
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
	void InitShared();
	void OpenShared();
	void RemoveShared();

	command_s* get_command();
	data_s* next_data(data_s* data_p);

	void CreateEventMain();
	void CreateEventDb();

	void CreateEventDebug();

	void OpenEventMain();
	void OpenEventDb();

	void OpenEventDebug();

	void SetEventMain();
	void SetEventDb();
	void SetEventDebug();

	void UnsetEventMain();
	void UnsetEventDb();
	void UnsetEventDebug();

	void CloseEvents();

	void syncMain();
	void syncDb();
	void syncDebug();
}
