#include <Control.hpp>
#include "../ControllerDll/Init.hpp" // to get DB_CNT and DbEnum

#include <odbc/Connection.h>
#include <odbc/Environment.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>
#include <odbc/Exception.h>

#ifndef __DEBUG__
	#define assert(p) ((void)0) // define mine
#else
	#define STRINGIZE(x) STRINGIZE2(x)
	#define STRINGIZE2(x) #x
	#define LINE_STRING STRINGIZE(__LINE__)

	#define assert(p) if (!(p)) {std::cerr << "Assertion failed on line " LINE_STRING; exit(1);}
#endif

#include <array>

#include <iostream>
#include <string>
#include <cstdarg>

static command_s* command_p;

static odbc::EnvironmentRef env = odbc::Environment::create();
static std::array<odbc::ConnectionRef, DB_CNT> conn_a;

std::string assemble_query(const char* query, ...)
{
    constexpr int max_query_size = 65536;
    static char query_buffer[max_query_size];

    va_list vl;
    va_start(vl, query);
    int len = vsprintf_s(query_buffer, sizeof(query_buffer), query, vl);
    va_end(vl);
    return std::string(query_buffer, len);
}


static void write_error(const char* err)
{
	data_s* data_p = Control::next_data(nullptr);
	data_p->type = DataType::Str;
	data_p->size = std::strlen(err) + 1;
	std::memcpy(data_p->body(), err, data_p->size);
}


static void initdb()
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
		command_p->cmd = Cmd::Done;
	} catch (odbc::Exception& e)
	{
		write_error(e.what());
		command_p->cmd = Cmd::Err;
	}
	Control::SetEventDb();
}


void cars()
{
	try
	{
		data_s* data_p = Control::next_data(nullptr);
		assert(data_p->type == DataType::Str);
		odbc::PreparedStatementRef ps 
			= conn_a[static_cast<int>(DBEnum::Cars)]->prepareStatement(
				assemble_query("SELECT weight, corr, gn FROM cars_table WHERE id='%s'", reinterpret_cast<const char*>(data_p->body())));
		odbc::ResultSetRef res_ref = ps->executeQuery();

		if (res_ref->next())
		{
			command_p->cmd = Cmd::Done;

			int weight = *res_ref->getInt(1);
			int corr = *res_ref->getInt(2);
			std::string gn = *res_ref->getString(3);

			data_p->type = DataType::Int;
			data_p->size = sizeof(int);
			*reinterpret_cast<int*>(data_p->body()) = weight;

			data_p = Control::next_data(data_p);
			data_p->type = DataType::Int;
			data_p->size = sizeof(int);
			*reinterpret_cast<int*>(data_p->body()) = corr;

			data_p = Control::next_data(data_p);
			data_p->type = DataType::Str;
			data_p->size = std::size(gn) + 1;
			std::memcpy(data_p->body(), gn.c_str(), data_p->size);
		}
		else
		{
			throw "No info recived";
		}
	}
	catch (std::exception& e)
	{
		write_error(e.what());
		command_p->cmd = Cmd::Err;
	}
	Control::SetEventDb();
}


static void store()
{
	data_s* data_p = Control::next_data(nullptr);
	assert(data_p->type == DataType::Str);
	const char* com_cstr = reinterpret_cast<const char*>(data_p->body());

	data_p = Control::next_data(data_p);
	assert(data_p->type == DataType::Int);
	int event_id = *reinterpret_cast<int*>(data_p->body());

	data_p = Control::next_data(data_p);
	assert(data_p->type == DataType::Str);
	const char* id_cstr = reinterpret_cast<const char*>(data_p->body());

	data_p = Control::next_data(data_p);
	assert(data_p->type == DataType::Int);
	int weight = *reinterpret_cast<int*>(data_p->body());

	data_p = Control::next_data(data_p);
	assert(data_p->type == DataType::Int);
	int inp_weight = *reinterpret_cast<int*>(data_p->body());

	odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::Store)]->prepareStatement(
		assemble_query("INSERT INTO info (com, event_id, id, weight, inp_weight) VALUES('%s', %d, '%s', %d, %d)", com_cstr, event_id, id_cstr, weight, inp_weight);

	try
	{
		ps->executeUpdate();
		command_p->cmd = Cmd::Done;
	}
	catch (const std::exception& e)
	{
		write_error(e.what());
		command_p->cmd = Cmd::Err;
	}
	Control::SetEventDb();
}


static int inc_event_id()
{
	auto ps = conn_a[static_cast<int>(DBEnum::Store)]->prepareStatement("SELECT nextval('event_id')");
	auto rs = ps->executeQuery();
	if (rs->next())
	{
		 return (int)*rs->getLong(1);
	}
	else
	{
		throw "Error in incrementing event id";
	}
}


static void store_info()
{
	try
	{
		int event_id = inc_event_id();

		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::Drivers)]->prepareStatement("SELECT fio FROM drivers WHERE id=?");

		data_s* data_p = Control::next_data(nullptr);
		assert(data_p->type == DataType::Str);
		ps->setCString(1, reinterpret_cast<const char*>(data_p->body()));

		odbc::ResultSetRef res_ref = ps->executeQuery();
		if (res_ref->next())
		{
			std::string fio = *res_ref->getString(1);

			data_p = Control::next_data(data_p);
			assert(data_p->type == DataType::Str);
			const char* com_cstr = reinterpret_cast<const char*>(data_p->body());

			data_p = Control::next_data(data_p);
			assert(data_p->type == DataType::Str);
			const char* barcode_cstr = reinterpret_cast<const char*>(data_p->body());

			data_p = Control::next_data(data_p);
			assert(data_p->type == DataType::Str);
			const char* gn_cstr = reinterpret_cast<const char*>(data_p->body());

			ps = conn_a[static_cast<int>(DBEnum::Store_Info)]->prepareStatement(
				assemble_query(
					"INSERT INTO info(event_id, com, barcode, gn, fio) VALUES(%d, '%s', '%s', '%s, '%s)"
					"ON CONFLICT (event_id) DO UPDATE SET "
					"event_id=EXCLUDED.event_id, com=EXCLUDED.com, barcode=EXCLUDED.barcode,"
					"gn=EXCLUDED.gn, fio=EXCLUDED.fio, ts=CURRENT_TIMESTAMP", event_id, com_cstr, barcode_cstr, gn_cstr, fio.c_str()));

			ps->executeUpdate();

			command_p->cmd = Cmd::Done;

			data_p = Control::next_data(nullptr);
			data_p->type = DataType::Int;
			data_p->size = sizeof(int);
			*reinterpret_cast<int*>(data_p->body()) = event_id;
		}
		else
		{
			throw "Error reciving fio from drivers db";
		}
	}
	catch (std::exception& e)
	{
		write_error(e.what());
		command_p->cmd = Cmd::Err;
	}
	Control::SetEventDb();
}


int main()
{
	Control::OpenShared();
	Control::OpenEventMain();
	Control::OpenEventDb();
	Control::OpenEventDebug();
#ifdef __DEBUG__
	Control::syncDebug();
#endif
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
			case Cmd::Read_Cars:
				cars();
				break;
			case Cmd::Store_Store:
				store();
				break;
			case Cmd::Store_Store_Info:
				store_info();
				break;
			case Cmd::Exit:
				goto main_end;
			default:
				assert(false);
		}
	}
main_end:
	return 0;
}
