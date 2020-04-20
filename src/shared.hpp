#pragma once
#include "msg.hpp"

#include <cstdio>
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
};


extern int log_level;					// fixer.cpp
extern bool debug;						// fixer.cpp
extern odbc::ConnectionRef debug_db;

template<typename ...Args> inline
void dprintf(const char *fmt, Args&& ...args)
{
	if (log_level > 0)
	{
		printf(fmt, args...);
	}
}


static void dprintf(msg_t msg)
{
	if (log_level > 0)
	{
		printf("%d: %s\n", std::get<0>(msg), std::get<1>(msg));
	}
}
