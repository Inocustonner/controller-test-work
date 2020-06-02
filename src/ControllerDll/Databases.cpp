#include "Databases.hpp"
#include "Output.hpp"
#include "State.hpp"
#include <Control.hpp>

#include <myassert.hpp>

#include <odbc/Environment.h>
#include <odbc/Connection.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

static odbc::EnvironmentRef env = odbc::Environment::create();
static odbc::ConnectionRef  store_db;
static odbc::ConnectionRef  store_info_db;
static odbc::ConnectionRef  cars_db;
static odbc::ConnectionRef  drivers_db;

// void store(const char* com, const char* id,
// 	int corr_weight, int inp_weight)
// {
// 	try
// 	{
// 		// id, code id, mass, timestamp(default like)
// 		odbc::PreparedStatementRef ps = store_db->prepareStatement("INSERT INTO info (com, event_id, id, weight, inp_weight) VALUES(?, ?, ?, ?, ?)");
// 		ps->setCString(1, com);
// 		ps->setInt(2, state.event_id);
// 		ps->setCString(3, id);
// 		ps->setInt(4, corr_weight);
// 		ps->setInt(5, inp_weight);
// 		ps->executeUpdate();
// 	}
// 	catch (const std::exception &e)
// 	{
// 		dprintf("Error while storing to store_db:\n\t%s\n", e.what());
// 		dprintf(msg<5>());
// 	}
// }


static void write_str_to_data(data_s* data_p, const char* cstr)
{
	data_p->type = DataType::Str;
	data_p->size = std::strlen(cstr) + 1;
	std::memcpy(data_p->body(), cstr, data_p->size);
}

inline
void write_int_to_data(data_s* data_p, int i)
{
	data_p->type = DataType::Int;
	data_p->size = sizeof(i);
	*reinterpret_cast<int*>(data_p->body()) = i;
}


void store(const char* com, const char* id,
	int corr_weight, int inp_weight)
{
	lockMutexStore();
	command_s* cmd_p = Control::get_command();
	cmd_p->cmd = Cmd::Store_Store;

	data_s* data_p = Control::next_data();
	write_str_to_data(data_p, com);

	data_p = Control::next_data(data_p);
	write_int_to_data(data_p, state.event_id);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, id);

	data_p = Control::next_data(data_p);
	write_int_to_data(data_p, corr_weight);

	data_p = Control::next_data(data_p);
	write_int_to_data(data_p, inp_weight);
	releaseMutexStore();
	Control::SetEventMain();
	// how to check errors?
}

// static void inc_event_id()
// {
// 	auto ps = store_db->prepareStatement("SELECT nextval('event_id')");
// 	auto rs = ps->executeQuery();
// 	if (rs->next())
// 	{
// 		state.event_id = (int)*rs->getLong(1);
// 	}
// }


// void store_info(const char* com, const char* barcode, const char* gn, const char* driver_id)
// {
// 	inc_event_id();

// 	auto ps = drivers_db->prepareStatement("SELECT fio FROM drivers WHERE id=?");
// 	ps->setCString(1, driver_id);
// 	auto res_ref = ps->executeQuery();

// 	if (res_ref->next())
// 	{
// 		std::string fio = *res_ref->getString(1);
// 		ps = store_info_db->prepareStatement(
// 			"INSERT INTO info(event_id, com, barcode, gn, fio) VALUES(?, ?, ?, ?, ?)"
// 			"ON CONFLICT (event_id) DO UPDATE SET "
// 			"event_id=EXCLUDED.event_id, com=EXCLUDED.com, barcode=EXCLUDED.barcode,"
// 			"gn=EXCLUDED.gn, fio=EXCLUDED.fio, ts=CURRENT_TIMESTAMP");

// 		ps->setInt(1, state.event_id);
// 		ps->setCString(2, com);
// 		ps->setCString(3, barcode);
// 		ps->setCString(4, gn);
// 		ps->setCString(5, fio.c_str());
// 		ps->executeUpdate();
// 	}
// 	else
// 	{
// 		dprintf("Failed to find driver with id = %s\n", driver_id);
// 		dprintf(msg<8>());
// 	}
// }

void store_info(const char* com, const char* barcode, const char* gn, const char* driver_id)
{
	command_s* cmd_p = Control::get_command();
	cmd_p->cmd = Cmd::Store_Store_Info;

	data_s* data_p = Control::next_data();
	write_str_to_data(data_p, driver_id);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, com);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, barcode);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, gn);

	// no need to wait
	Control::SetEventMain();
	Control::syncDb();
	if (cmd_p->cmd == Cmd::Done)
	{
		data_p = Control::next_data();
		massert(data_p->type == DataType::Int);
		state.event_id = *reinterpret_cast<int*>(data_p->body());
	}
	else
	{
		fprintf(stderr, "%s", reinterpret_cast<const char*>(data_p->body()));
	}
}

// odbc::ResultSetRef select_from_cars()
// {
// 	odbc::PreparedStatementRef ps = cars_db->prepareStatement("SELECT weight, corr, gn FROM cars_table WHERE id=?");
// 	ps->setString(1, state.id);
// 	return ps->executeQuery();
// }

data_s* select_from_cars()
{
	command_s* cmd_p = Control::get_command();
	data_s* data_p = Control::next_data(nullptr);

	cmd_p->cmd = Cmd::Read_Cars;
	data_p->type = DataType::Str;
	data_p->size = std::size(state.id) + 1;
	std::memcpy(data_p->body(), state.id.c_str(), data_p->size);

	Control::SetEventMain();
	Control::syncDb();
	if (cmd_p->cmd == Cmd::Done)
	{
		return Control::next_data(nullptr);
	}
	else
	{
		fprintf(stderr, "%s", reinterpret_cast<const char*>(data_p->body()));
		return nullptr;
	}
}

// void set_cars_db(odbc::ConnectionRef new_cars_db)
// {
// 	cars_db = new_cars_db;
// }


// void set_store_db(odbc::ConnectionRef new_store_db)
// {
// 	store_db = new_store_db;
// }


// void set_store_info_db(odbc::ConnectionRef new_store_info_db)
// {
// 	store_info_db = new_store_info_db;
// }


odbc::ConnectionRef& get_store_db()
{
	return store_db;
}


odbc::ConnectionRef& get_store_info_db()
{
	return store_info_db;
}


odbc::ConnectionRef& get_cars_db()
{
	return cars_db;
}


odbc::ConnectionRef& get_drivers_db()
{
	return drivers_db;
}


odbc::EnvironmentRef get_odbc_env()
{
	return env;
}
