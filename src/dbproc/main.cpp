#include <Control.hpp>
#include "../ControllerDll/Init.hpp" // to get DB_CNT and DbEnum

#include <odbc/Connection.h>
#include <odbc/Environment.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>
#include <odbc/Exception.h>

#include <myassert.hpp>

#include <array>

#include <iostream>
#include <string>
#include <cstdarg>

static command_s* command_p;

static odbc::EnvironmentRef env = odbc::Environment::create();
static std::array<odbc::ConnectionRef, DB_CNT> conn_a;

const char* assemble_query(const char* query, ...)
{
    constexpr int max_query_size = 65536;
    static char query_buffer[max_query_size];

    va_list vl;
    va_start(vl, query);
    int len = vsprintf_s(query_buffer, sizeof(query_buffer), query, vl);
    va_end(vl);
    return query_buffer;
}

static void debug();

static void write_error(const char* err, bool to_db = true)
{
	data_s* data_p = Control::next_data(nullptr);
	int len = std::strlen(err) + 1;
	if (to_db)
	{
		data_p->type = DataType::Int;
		data_p->size = sizeof(int);
		*reinterpret_cast<int*>(data_p->body());

		data_p->type = DataType::Str;
		data_p->size = len;
		std::memcpy(data_p->body(), err, data_p->size);
		debug();
	}
	data_p = Control::next_data(nullptr);
	data_p->type = DataType::Str;
	data_p->size = len;
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
			massert(data_p->type == DataType::Int);
			odbc::ConnectionRef& conn = conn_a[*reinterpret_cast<int*>(data_p->body())];
			conn = env->createConnection();

			data_p = Control::next_data(data_p);
			massert(data_p->type == DataType::Str);
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
		massert(data_p->type == DataType::Str);
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
	massert(data_p->type == DataType::Str);
	const char* com_cstr = reinterpret_cast<const char*>(data_p->body());

	data_p = Control::next_data(data_p);
	massert(data_p->type == DataType::Int);
	int event_id = *reinterpret_cast<int*>(data_p->body());

	data_p = Control::next_data(data_p);
	massert(data_p->type == DataType::Str);
	const char* id_cstr = reinterpret_cast<const char*>(data_p->body());

	data_p = Control::next_data(data_p);
	massert(data_p->type == DataType::Int);
	int weight = *reinterpret_cast<int*>(data_p->body());

	data_p = Control::next_data(data_p);
	massert(data_p->type == DataType::Int);
	int inp_weight = *reinterpret_cast<int*>(data_p->body());
	try
	{
		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::Store)]->prepareStatement(
			assemble_query("INSERT INTO info (com, event_id, id, weight, inp_weight) VALUES('%s', %d, '%s', %d, %d)", com_cstr, event_id, id_cstr, weight, inp_weight));

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
	// SELECT * FROM sys.sequences WHERE name = 'event_id';
	// "SELECT nextval('event_id')" in postgres
	auto ps = conn_a[static_cast<int>(DBEnum::Store)]->prepareStatement("SELECT NEXT VALUE FOR event_id");
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

		data_s* data_p = Control::next_data(nullptr);
		massert(data_p->type == DataType::Str);

		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::Drivers)]->prepareStatement(assemble_query("SELECT fio FROM drivers WHERE id=%s",
				reinterpret_cast<const char*>(data_p->body())));

		odbc::ResultSetRef res_ref = ps->executeQuery();
		if (res_ref->next())
		{
			std::string fio = *res_ref->getString(1);

			data_p = Control::next_data(data_p);
			massert(data_p->type == DataType::Str);
			const char* com_cstr = reinterpret_cast<const char*>(data_p->body());

			data_p = Control::next_data(data_p);
			massert(data_p->type == DataType::Str);
			const char* barcode_cstr = reinterpret_cast<const char*>(data_p->body());

			data_p = Control::next_data(data_p);
			massert(data_p->type == DataType::Str);
			const char* gn_cstr = reinterpret_cast<const char*>(data_p->body());
			const char* query_templ =
				"IF EXISTS(SELECT * FROM info WHERE event_id=%d)\n"
				"UPDATE info SET com='%s', barcode='%s', gn='%s', fio='%s' WHERE event_id=%d;\n"
				"ELSE\n"
				"INSERT INTO info(event_id, com, barcode, gn, fio) VALUES(%d, '%s', '%s', '%s', '%s');";
			ps = conn_a[static_cast<int>(DBEnum::Store_Info)]->prepareStatement(
				assemble_query(
				    query_templ,
					event_id,
					com_cstr, barcode_cstr, gn_cstr, fio.c_str(), event_id,
					event_id, com_cstr, barcode_cstr, gn_cstr, fio.c_str()));

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


static void debug()
{
	data_s* data_p = Control::next_data(nullptr);
	massert(data_p->type == DataType::Int);
	int code = *reinterpret_cast<int*>(data_p->body());

	data_p = Control::next_data(data_p);
	massert(data_p->type == DataType::Str);
	const char* str = reinterpret_cast<const char*>(data_p->body());

	try
	{
		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::Debug)]->prepareStatement(
			assemble_query("INSERT INTO debug(code, message) VALUES(%d, '%s')", code, str));
		ps->executeUpdate();

		command_p->cmd = Cmd::Done;
	}
	catch(odbc::Exception& e)
	{
		write_error(e.what(), false);
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
	Control::OpenMutexStore();
	Control::OpenMutexDebug();

#ifdef __DEBUG__
	Control::syncDebug();
#endif
	command_p = Control::get_command();

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
			case Cmd::Store_Debug:
				if (conn_a[static_cast<int>(DBEnum::Debug)]->connected())
					debug();
				break;
			case Cmd::Exit:
				goto main_end;
			default:
				massert(false);
		}
	}
main_end:
	return 0;
}
