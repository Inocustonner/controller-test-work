#include "fixer.h"
#include "serialPool.hpp"
#include <dllInjLib/dllInj.h>	// https://github.com/Inocustonner/dllInjectionLib

#include <odbc/Connection.h>	// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Environment.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

#include <inipp.h>

#include <string_view>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <fstream>
#include <cstdarg>

odbc::EnvironmentRef odbc_env;
odbc::ConnectionRef cars_db, store_db, store_info_db;
// in cars_db cars table is used
// in store_db info table is used
std::thread reader_thread;		// thread that reads from serial port


std::atomic<bool> authorized = false;	// has this driver pass authorization
bool debug = false;

auto dprintf(const char *fmt, ...)
{
	if (debug)
	{
		va_list al;
		va_start(al, fmt);
		int rc = vprintf(fmt, al);
		va_end(al);
		return rc;
	}
	return 0;
}

struct PortInfo
{
	std::string name;			// "COM1" for example
	size_t baudrate					= 9600;
	serial::bytesize_t bytesize		= serial::bytesize_t::eightbits; // fivebits, sixbits, sevenbits, eightbits
	serial::parity_t	 parity		= serial::parity_t::parity_none;  // parity_none, parity_even, parity_odd
	serial::stopbits_t stopbits		= serial::stopbits_t::stopbits_one; // stopbits_one, stopbits_one_point_five, stopits_two
};

struct DbAuth
{
	std::string server, port, db, uid, pwd;
};

struct Settings
{
	std::vector<PortInfo> pi;				// contains info about ports
	DbAuth cars, store, store_info;
};

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
}state;


inline
void reset_state()
{
	state = State{};
	authorized = false;
}


void store(double p1)
{
	try
	{
		// id, code id, mass, timestamp(default like)
		odbc::PreparedStatementRef ps = store_db->prepareStatement("INSERT INTO info (event_id, id, weight, inp_weight) VALUES(?, ?, ?, ?)");
		ps->setInt(1, state.event_id);
		ps->setString(2, state.id);
		ps->setInt(3, (int)(p1 - state.corr));
		ps->setInt(4, (int)p1);
		ps->executeUpdate();
	}
	catch (const std::exception &e)
	{
		dprintf("%s\n", e.what());
	}
}


inline
double phase1(double p1, bool is_stable)
{
	return p1 - state.corr;
}


inline
void phase0(double p1)
{
	if (state.p0 > p1 && state.p0 > state.reset_thr) // reset on entering
	{
		dprintf("resetting\n");
		reset_state();
	}
}


double fix(double p1, bool is_stable)
{
	double ret_value = p1;
	if (authorized)
	{
		if (p1 < state.min_weight)
		{
			state.phase = 0;
			phase0(p1);
		}
		else
		{
			state.phase = 1;
			ret_value = phase1(p1, is_stable);
		}

		if (std::abs(p1 - state.p0) > state.store_diff && is_stable)
		{
			store(p1);
		}
	}
	else if (p1 >= state.reset_thr)
	{
		dprintf("Unauthorized driver\n");
	}	

	state.p0 = p1;
	return ret_value;		
}


inline
void store_info(const std::string &com, const std::string &barcode)
{
	//															 event_id, com, barcode(event_id yet), (default timestamp)
	odbc::PreparedStatementRef ps = store_info_db->prepareStatement("INSERT INTO info VALUES(?, ?, ?)"\
																 "ON CONFLICT (event_id) DO UPDATE SET "
																 "event_id=EXCLUDED.event_id, com=EXCLUDED.com, barcode=EXCLUDED.barcode, ts=CURRENT_TIMESTAMP");
	ps->setInt(1, state.event_id);
	ps->setString(2, com);
	ps->setString(3, barcode);
	ps->executeUpdate();
}


inline
void inc_event_id()
{
	auto ps = store_db->prepareStatement("SELECT nextval('event_id')");
	auto rs = ps->executeQuery();
	if (rs->next())
	{
		state.event_id = (int)*rs->getLong(1);
	}
}


bool is_barcode_valid(std::string_view barcode)
{
	return true;
}


void serial_read(std::vector<PortInfo> pi)
{
	std::vector<serial::Serial> ports(std::size(pi)); // used once
	for (size_t i = 0; i < std::size(pi); ++i)
	{
		ports[i].setPort(pi[i].name);
		ports[i].setBaudrate(pi[i].baudrate);
		ports[i].setBytesize(pi[i].bytesize);
		ports[i].setParity(pi[i].parity);
		ports[i].open();
		if (!ports[i].isOpen())
			dprintf("Error oppening port %s\n", ports[i].getPort().c_str());
	}

	try
	{
		SerialPool serial_pool{ std::move(ports) };
		while (true)
		{
			serial::Serial &serial_port = serial_pool.bad_wait();
			const size_t max_line_sz = 65536;
			if (authorized)
			{
				dprintf("Error double authorization\n");
				serial_port.readline(max_line_sz, "\r"); // deny given data
				continue;
			}

			try
			{
				std::string barcode = serial_port.readline(max_line_sz, "\r"); // from bar code
				barcode.pop_back(); // remove eol symbol
				dprintf("%s: read %s\n", serial_port.getPort().c_str(), barcode.c_str());
				
				if (!is_barcode_valid(barcode))
				{
					dprintf("Invalid barcode\n");
					continue;
				}
				state.id = barcode;

				odbc::PreparedStatementRef ps = cars_db->prepareStatement("SELECT weight, corr FROM cars_table WHERE id=?");
				ps->setString(1, state.id);
				odbc::ResultSetRef rs = ps->executeQuery();

				if (rs->next())
				{
					inc_event_id();

					// save info about authorization
					store_info(serial_port.getPort(), barcode);

					state.min_weight = (double)*rs->getInt(1);
					state.corr = (double)*rs->getInt(2);
					
					authorized = true;
				}
			}
			catch (const std::exception &e)
			{
				dprintf("%s\n", e.what());
			}
		}
	}
	catch (const std::exception &e)
	{
		dprintf("%s\n", e.what());
		system("pause");
		exit(0);
	}
}


bool init_db(Settings set)
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
	}
	catch (const odbc::Exception &e)
	{
		dprintf("%s\n", e.what());
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


Settings read_settings(std::string ini_path)
{
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
		else if (it->first == "reset_thr")
		{
			iss >> state.reset_thr;
			dprintf("reset_thr %lf\n", state.reset_thr);
		}
		else if (it->first == "store_diff")
		{
			iss >> state.store_diff;
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
		for (auto it = std::begin(ini.sections["DEBUG"]); it != std::end(ini.sections["DEBUG"]); ++it)
		{
			if (it->first == "Console" && it->second == "en")
			{
				debug = true;
				CreateConsole();
			}
		}
	}
	return set;
}


bool init_fixer(const char *ini_filename)
{
	Settings set = read_settings(path_to_ini(ini_filename));
	if (!init_db(set))
	{
		return false;
	}

	reset_state();
	
	reader_thread = std::thread(serial_read, set.pi);
	reader_thread.detach();
	return true;
}
