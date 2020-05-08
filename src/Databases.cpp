#include "Databases.hpp"
#include "Output.hpp"
#include "State.hpp"

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

void store(const char* com, const char* id,
	int corr_weight, int inp_weight) noexcept
{
	try
	{
		// id, code id, mass, timestamp(default like)
		odbc::PreparedStatementRef ps = store_db->prepareStatement("INSERT INTO info (com, event_id, id, weight, inp_weight) VALUES(?, ?, ?, ?, ?)");
		ps->setCString(1, com);
		ps->setInt(2, state.event_id);
		ps->setCString(3, id);
		ps->setInt(4, corr_weight);
		ps->setInt(5, inp_weight);
		ps->executeUpdate();
	}
	catch (const std::exception &e)
	{
		dprintf("Error while storing to store_db:\n\t%s\n", e.what());
		dprintf(msg<5>());
	}
}


static void inc_event_id()
{
	auto ps = store_db->prepareStatement("SELECT nextval('event_id')");
	auto rs = ps->executeQuery();
	if (rs->next())
	{
		state.event_id = (int)*rs->getLong(1);
	}
}


void store_info(const char* com, const char* barcode, const char* gn, const char* driver_id)
{
	inc_event_id();

	auto ps = drivers_db->prepareStatement("SELECT fio FROM drivers WHERE id=?");
	ps->setCString(1, driver_id);
	auto res_ref = ps->executeQuery();

	if (res_ref->next())
	{
		std::string fio = *res_ref->getString(1);
		ps = store_info_db->prepareStatement(
			"INSERT INTO info(event_id, com, barcode, gn, fio) VALUES(?, ?, ?, ?, ?)"
			"ON CONFLICT (event_id) DO UPDATE SET "
			"event_id=EXCLUDED.event_id, com=EXCLUDED.com, barcode=EXCLUDED.barcode,"
			"gn=EXCLUDED.gn, fio=EXCLUDED.fio, ts=CURRENT_TIMESTAMP");

		ps->setInt(1, state.event_id);
		ps->setCString(2, com);
		ps->setCString(3, barcode);
		ps->setCString(4, gn);
		ps->setCString(5, fio.c_str());
		ps->executeUpdate();
	}
	else
	{
		dprintf("Failed to find driver with id = %s\n", driver_id);
	}
}


odbc::ResultSetRef select_from_cars()
{
	odbc::PreparedStatementRef ps = cars_db->prepareStatement("SELECT weight, corr, gn FROM cars_table WHERE id=?");
	ps->setString(1, state.id);
	return ps->executeQuery();
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
