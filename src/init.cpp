#include "shared.hpp"
#include "init.hpp"
#include <dllInjLib/dllInj.h>	// https://github.com/Inocustonner/dllInjectionLib

#include <inipp.h>
#include <fstream>

#define DEFAULT_LOG_LEVEL 1

bool init_db(Settings set, odbc::EnvironmentRef &odbc_env,
			 odbc::ConnectionRef &cars_db,
			 odbc::ConnectionRef &store_db,
			 odbc::ConnectionRef &store_info_db,
			 odbc::ConnectionRef &debug_db)
{
	try
	{
		odbc_env = odbc::Environment::create();
		cars_db = odbc_env->createConnection();
		// const char *cars_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=cars; UID=postgres; PWD=root;";
		std::string cars_conn_str = "DRIVER={PostgreSQL ANSI};"
			"SERVER=" + set.cars.server +
			";PORT=" + set.cars.port +
			";DATABASE=" + set.cars.db +
			";UID=" + set.cars.uid +
			";PWD=" + set.cars.pwd;

		cars_db->connect(cars_conn_str.c_str());

		store_db = odbc_env->createConnection();
		// const char *store_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=store; UID=postgres; PWD=root;";
		std::string store_conn_str = "DRIVER={PostgreSQL ANSI};"
			"SERVER=" + set.store.server +
			";PORT=" + set.store.port +
			";DATABASE=" + set.store.db +
			";UID=" + set.store.uid +
			";PWD=" + set.store.pwd;
		
		store_db->connect(store_conn_str.c_str());

		store_info_db = odbc_env->createConnection();
		// const char *store_info_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=store_info; UID=postgres; PWD=root;";
		std::string store_info_conn_str = "DRIVER={PostgreSQL ANSI};"
			"SERVER=" + set.store_info.server +
			";PORT=" + set.store_info.port +
			";DATABASE=" + set.store_info.db +
			";UID=" + set.store_info.uid +
			";PWD=" + set.store_info.pwd;

		store_info_db->connect(store_info_conn_str.c_str());

		debug_db = odbc_env->createConnection();
		// const char *cars_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=cars; UID=postgres; PWD=root;";
		std::string debug_conn_str = "DRIVER={PostgreSQL ANSI};"
			"SERVER=" + set.debug.server +
			";PORT=" + set.debug.port +
			";DATABASE=" + set.debug.db +
			";UID=" + set.debug.uid +
			";PWD=" + set.debug.pwd;

		debug_db->connect(debug_conn_str.c_str());
	}
	catch (const odbc::Exception &e)
	{
		dprintf("Error while connecting to dbs:\n\t%s", e.what());
		return false;
	}
	return true;
}


std::string path_to_ini(std::string ini_filename)
{
	std::string ini_path = GetCurrentProcPath();
	size_t path_len = ini_path.find_last_of("\\") + 1;
	ini_path.replace(path_len, ini_path.size() - path_len, ini_filename);
	return ini_path;
}


Settings read_settings(const char *ini_filename, State &state)
{
	std::string ini_path = path_to_ini(ini_filename);
	dprintf("Path to ini:\n\t%s\n", ini_path.c_str());
	inipp::Ini<char> ini;
	std::ifstream is(ini_path);
	ini.parse(is);

	Settings set;
	for (auto it = std::begin(ini.sections["DEFAULT"]); it != std::end(ini.sections["DEFAULT"]); ++it)
	{
		std::istringstream iss{ it->second };
		if (it->first == "carsdb")
		{
			iss >> set.cars.server >> set.cars.port >> set.cars.db >> set.cars.uid >> set.cars.pwd;
		}
		else if (it->first == "storedb")
		{
			iss >> set.store.server >> set.store.port >> set.store.db >> set.store.uid >> set.store.pwd;
		}
		else if (it->first == "store_infodb")
		{
			iss >> set.store_info.server >> set.store_info.port >> set.store_info.db >> set.store_info.uid >> set.store_info.pwd;
		}
		if (it->first == "debugdb")
		{
			iss >> set.debug.server >> set.debug.port >> set.debug.db >> set.debug.uid >> set.debug.pwd;
		}
		else if (it->first == "reset_thr")
		{
			iss >> state.reset_thr;
			dprintf("reset_thr set to %lf\n", state.reset_thr);
		}
		else if (it->first == "store_diff")
		{
			iss >> state.store_diff;
			dprintf("store_diff set to %lf\n", state.store_diff);
		}
		else
		{
			dprintf("unkonwn option is %s ignored\n", it->first.c_str());
		}
	}

	for (auto it = std::begin(ini.sections["COM"]); it != std::end(ini.sections["COM"]); ++it)
	{
		PortInfo pi;
		std::stringstream is(it->second);

		pi.name = it->first;
		size_t baudrate = 0, bytesize = 0;
		std::string parity = "";
		is >> baudrate >> bytesize >> parity;

		// check baudrate
		if (baudrate)
		{
			switch (baudrate)
			{
				case 9600:
				case 19200:
				case 38400:
				case 57600:
				case 115200:
					pi.baudrate = baudrate;
					break;

				default:
					dprintf("%s: Baudrate %u is unusual\n", pi.name.c_str(), (pi.baudrate = baudrate));
			}
		}
		
		if (bytesize)
		{
			switch (bytesize)
			{
				case 5:
				case 6:
				case 7:
				case 8:
					pi.bytesize = static_cast<serial::bytesize_t>(bytesize);
					break;

				default:
					dprintf("%s: Invalid bytesize %u. Available [5-8] values. Set to default %u.", pi.name.c_str(), bytesize, static_cast<unsigned int>(pi.bytesize));
			}
		}

		if (std::size(parity))
		{
			if (parity == "even")
			{
				pi.parity = serial::parity_t::parity_even;
			}
			else if (parity == "odd")
			{
				pi.parity = serial::parity_t::parity_odd;
			}
			else if (parity == "none")
			{
				pi.parity = serial::parity_t::parity_none;
			}
			else
			{
				dprintf("%s: Invalid parity value %s. Available \"even\", \"odd\", \"none\". Set to default \"none\"\n", pi.name.c_str(), parity.c_str());
			}
		}

		set.pi.push_back(pi);
	}

	if (ini.sections.find("DEBUG") != std::end(ini.sections))
	{
		debug = true;
		for (auto it = std::begin(ini.sections["DEBUG"]); it != std::end(ini.sections["DEBUG"]); ++it)
		{
			std::stringstream is(it->second);
			if (it->first == "LogLevel")
			{
				is >> log_level;
				if (log_level > 0)
					CreateConsole();

				if (log_level > 3)
				{
					log_level = DEFAULT_LOG_LEVEL;
					dprintf("Invalid log_level.\nSet to default (%d)\n", log_level);
				}
			}
		}
	}
	return set;
}
