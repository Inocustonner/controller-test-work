#include "Databases.hpp"
#include "Output.hpp"

#include <odbc/Connection.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>

static odbc::ConnectionRef store_db;

void store(const char *com, const char *id,
	int event_id, int corr_weight, int inp_weight) noexcept
{
	try
	{
		// id, code id, mass, timestamp(default like)
		odbc::PreparedStatementRef ps = store_db->prepareStatement("INSERT INTO info (com, event_id, id, weight, inp_weight) VALUES(?, ?, ?, ?, ?)");
		ps->setCString(1, com);
		ps->setInt(2, event_id);
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

void set_store_db(odbc::ConnectionRef&& new_store_db)
{
	store_db = std::move(new_store_db);
}
