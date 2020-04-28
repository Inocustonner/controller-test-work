#pragma once
#include "msg.hpp"
#include <mmsg/mmsg.hpp>

#include <cstdio>
#include <array>
#include <string>

#include <odbc/Connection.h>	// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Environment.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

struct State
{
	double reset_thr					= 15000.0; // the value on scalars below what do reset
	double store_diff					= 200.0;   // if difference between values on scalars  equals store_diff store those values

	double min_weight					= 1000.0;	// set on read_serial. Value after what apply correction
	double corr						= 0.0;		// correction value
	int event_id						= 0;
	double p0							= 0.0;		// previous value on scalars
	int phase							= 0;		// current phase
	std::string id					= "";		// car ident
	std::string com					= "";

	std::string lights_wait_cmd		= "";	
	std::string lights_acc_cmd		= "";
	std::string lights_deny_cmd		= "";
};


// log_level:
// 	0 - no console, db
// 	1 - console, db
// 	2 - console, no db
extern int log_level;					// fixer.cpp
extern bool debug;						// fixer.cpp
extern odbc::ConnectionRef debug_db;


template<typename ...Args> inline
void dbprint(const char *fmt, int code, Args&& ...args)
{
	try
	{
		static std::array<char, 1024> buf;
		buf.fill('\0');			// zero buf
		snprintf(buf.data(), std::size(buf) - 1, fmt, code, args...);
		auto ps = debug_db->prepareStatement("INSERT INTO debug(code, message) VALUES(?, ?)"
											 "ON CONFLICT (id) DO UPDATE SET "
											 "id=EXCLUDED.id, code=EXCLUDED.code, message=EXCLUDED.message, ts=EXCLUDED.ts");
		ps->setInt(1, code);
		ps->setCString(2, buf.data());
		ps->executeUpdate();
	}
	catch (const odbc::Exception &e)
	{
		printf("Error while storing to debug db:\n\t%s", e.what());
	}
}


template<typename ...Args> inline
void dprintf(const char *fmt, Args&& ...args)
{
	if (log_level > 0)
	{
		printf(fmt, args...);
	}
	if (log_level < 2)
	{
		dbprint(fmt, -1, std::forward<Args>(args)...);
	}
}

// add msg box
static void dprintf(msg_t msg)
{
	static int last_msg = -1;

	if (last_msg != std::get<0>(msg))
	{
		last_msg = std::get<0>(msg);
		if (log_level > 0)
		{
			printf("%d: %s\n", std::get<0>(msg), std::get<1>(msg));
		}
		if (log_level < 2)
		{
			dbprint("%d: %s", std::get<0>(msg), std::get<1>(msg));
		}
		mMsgBox(std::get<2>(msg), L"ERROR", 20000);
	}
}
