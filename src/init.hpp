#pragma once

#include "serialPool.hpp"

struct PortInfo
{
	std::string name;													// "COM1" for example
	size_t baudrate					= 9600;
	serial::bytesize_t bytesize		= serial::bytesize_t::eightbits;	// fivebits, sixbits, sevenbits, eightbits
	serial::parity_t parity			= serial::parity_t::parity_none;	// parity_none, parity_even, parity_odd
	serial::stopbits_t stopbits		= serial::stopbits_t::stopbits_one; // stopbits_one, stopbits_one_point_five, stopits_two
};

struct DbAuth
{
	std::string server, port, db, uid, pwd;
};

struct Settings
{
	std::vector<PortInfo> pi;				// contains info about ports
	DbAuth cars, store, store_info, debug;
};


bool init_db(Settings set, odbc::EnvironmentRef &odbc_env,
			 odbc::ConnectionRef &cars_db,
			 odbc::ConnectionRef &store_db,
			 odbc::ConnectionRef &store_info_db,
			 odbc::ConnectionRef &debug_db);

Settings read_settings(const char *ini_filename, State &state);
