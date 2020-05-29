#include <Control.hpp>
#include "../ControllerDll/Init.hpp" // to get DB_CNT

#include <odbc/Connection.h>
#include <odbc/Environment.h>
#include <odbc/Exception.h>

#define assert() ((void)0) // define mine

#include <array>

#include <iostream>
#include <string>

static command_s* command_p;

static odbc::EnvironmentRef env = odbc::Environment::create();
static std::array<odbc::ConnectionRef, DB_CNT> conn_a;

// MAKE SHUTDOWN ROUTINE

inline
void initdb()
{
	// EXPTECT:
	// each data_s contains: 1 - db id, 2 - db connection string
	try
	{
		data_s* data_p = Control::next_data(nullptr);
		while (data_p->size)
		{
			assert(data_p->type == DataType::Int);
			odbc::ConnectionRef& conn = conn_a[*reinterpret_cast<int*>(data_p->body())];
			conn = env->createConnection();

			data_p = Control::next_data(data_p);
			assert(data_p->type == DataType::Str);
			conn->connect(reinterpret_cast<const char*>(data_p->body()));

			data_p = Control::next_data(data_p);
		}
	} catch (odbc::Exception& e)
	{
		std::cerr << e.what() << '\n';
		system("pause");
		exit(1);
	}
	command_p->cmd = Cmd::Done;

	Control::SetEventDb();
}


int main()
{
	Control::OpenShared();
	Control::OpenEventMain();
	Control::OpenEventDb();
	Control::OpenEventDebug();

	Control::syncDebug();

	command_p = Control::get_command();
	Control::UnsetEventDb();

	while (true)
	{
		Control::syncMain();
		switch (command_p->cmd)
		{
			case Cmd::InitDb:
				initdb();
				break;
			default:
				assert(false);
		}
	}
	return 0;
}