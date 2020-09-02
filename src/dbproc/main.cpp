#include <Control.hpp>
#include "../ControllerDll/core/Init.hpp" // to get DB_CNT and DbEnum

#include <odbc/Connection.h>
#include <odbc/Environment.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>
#include <odbc/Exception.h>
#include <odbc/DatabaseMetaData.h>

#include <myassert.hpp>

#include <array>

#include <iostream>
#include <string>
#include <cstdarg>

enum class DbProvider
{
	PostgreSQL,
	MSSQL
};

DbProvider dbprovider;

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
	data_s* reason_p = nullptr;
	int len = std::strlen(err) + 1;
	if (to_db)
	{
		data_p->type = DataType::Int;
		data_p->size = sizeof(int);
		*reinterpret_cast<int*>(data_p->body()) = -1;

		reason_p = Control::next_data(data_p);
		reason_p->type = DataType::Str;
		reason_p->size = len;
		std::memcpy(reason_p->body(), err, reason_p->size);
		debug();
	}
	reason_p = Control::next_data(nullptr);
	reason_p->type = DataType::Str;
	reason_p->size = len;
	std::memcpy(reason_p->body(), err, reason_p->size);
}


static void initdb()
{
	// EXPTECT:
	// each data_s contains: 1 - db id, 2 - db connection string
	try
	{
		data_s* data_p = Control::next_data(nullptr);
		massert(data_p->type == DataType::Str);
		const std::string db_provider(reinterpret_cast<const char*>(data_p->body()));
		if (db_provider == "PostgreSQL")
		{
			dbprovider = DbProvider::PostgreSQL;
		}
		else
		{
			dbprovider = DbProvider::MSSQL;
		}

		data_p = Control::next_data(data_p);

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
			= conn_a[static_cast<int>(DBEnum::W_Ext)]->prepareStatement(
				assemble_query("SELECT weight, corr, gn FROM cars WHERE id='%s'", reinterpret_cast<const char*>(data_p->body())));
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
			throw std::exception("No info recived");
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
		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::W_Ext)]->prepareStatement(
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
	odbc::PreparedStatementRef ps;

	if (dbprovider == DbProvider::PostgreSQL)
		ps = conn_a[static_cast<int>(DBEnum::W_Ext)]->prepareStatement("SELECT nextval('event_id')");
	else if (dbprovider == DbProvider::MSSQL)
		ps = conn_a[static_cast<int>(DBEnum::W_Ext)]->prepareStatement("SELECT NEXT VALUE FOR event_id");

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

		data_s* data_p = Control::next_data();
		massert(data_p->type == DataType::Int);
		int allow_udentified = *static_cast<int*>(data_p->body());

		data_p = Control::next_data(data_p);
		massert(data_p->type == DataType::Str);

		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::W_Ext)]->prepareStatement(assemble_query("SELECT fio FROM drivers WHERE id=%s",
				reinterpret_cast<const char*>(data_p->body())));

		odbc::ResultSetRef res_ref = ps->executeQuery();
		std::string fio;
		if (res_ref->next())
		{
			fio = *res_ref->getString(1);
		}
		else if (allow_udentified)
		{
			fio = "";
		}
		else
		{
			throw std::exception("Error reciving fio from drivers db");
		}
		data_p = Control::next_data(data_p);
		massert(data_p->type == DataType::Str);
		const char* com_cstr = reinterpret_cast<const char*>(data_p->body());

		data_p = Control::next_data(data_p);
		massert(data_p->type == DataType::Str);
		const char* barcode_cstr = reinterpret_cast<const char*>(data_p->body());

		data_p = Control::next_data(data_p);
		massert(data_p->type == DataType::Str);
		const char* gn_cstr = reinterpret_cast<const char*>(data_p->body());
		const char* begin_postgres = "DO $$\nBEGIN\n";
		const char* end_postgres = "\nEND IF; END\n$$";
		const char* query_templ =
			"%s"
			"IF EXISTS(SELECT * FROM info WHERE event_id=%d) %s\n"
			"UPDATE info SET com='%s', barcode='%s', gn='%s', fio='%s' WHERE event_id=%d;\n"
			"ELSE\n"
			"INSERT INTO info(event_id, com, barcode, gn, fio) VALUES(%d, '%s', '%s', '%s', '%s');"
			"%s";
		ps = conn_a[static_cast<int>(DBEnum::W_Base)]->prepareStatement(
			assemble_query(
				query_templ,
				dbprovider == DbProvider::PostgreSQL ? begin_postgres : "",
				event_id, dbprovider == DbProvider::PostgreSQL ? "THEN" : "",
				com_cstr, barcode_cstr, gn_cstr, fio.c_str(), event_id,
				event_id, com_cstr, barcode_cstr, gn_cstr, fio.c_str(),
				dbprovider == DbProvider::PostgreSQL ? end_postgres : "" ));

		ps->executeUpdate();

		command_p->cmd = Cmd::Done;

		data_p = Control::next_data(nullptr);
		data_p->type = DataType::Int;
		data_p->size = sizeof(int);
		*reinterpret_cast<int*>(data_p->body()) = event_id;
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
	if (conn_a[static_cast<int>(DBEnum::W_Ext)].isNull() ||
		!conn_a[static_cast<int>(DBEnum::W_Ext)]->connected())
		goto end_sync;

	data_s* data_p = Control::next_data(nullptr);
	massert(data_p->type == DataType::Int);
	int code = *reinterpret_cast<int*>(data_p->body());

	data_p = Control::next_data(data_p);
	massert(data_p->type == DataType::Str);
	const char* str = reinterpret_cast<const char*>(data_p->body());

	try
	{
		odbc::PreparedStatementRef ps = conn_a[static_cast<int>(DBEnum::W_Ext)]->prepareStatement(
			assemble_query("INSERT INTO debug(code, message) VALUES(%d, '%s')", code, str));
		ps->executeUpdate();

		command_p->cmd = Cmd::Done;
	}
	catch(odbc::Exception& e)
	{
		write_error(e.what(), false);
		command_p->cmd = Cmd::Err;
	}
end_sync:
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

	Control::syncDebug();

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
				if (conn_a[static_cast<int>(DBEnum::W_Ext)]->connected())
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
