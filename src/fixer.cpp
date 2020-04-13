#include "fixer.h"
#include "serialPool.hpp"
#include <dllInjLib/dllInj.h>	// https://github.com/Inocustonner/dllInjectionLib

#include <odbc/Connection.h>	// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Environment.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

#include <string_view>
#include <thread>
#include <cstdio>
#include <cstdlib>

odbc::EnvironmentRef odbc_env;
odbc::ConnectionRef cars_db, store_db, store_info_db;
// in cars_db cars table is used
// in store_db info table is used
std::thread reader_thread;		// thread that reads from serial port


struct PortInfo
{
	std::string name;			// "COM1" for example
	size_t baudrate;
};


struct State
{
	double min_weight		= 1000.0;
	double corr				= 0.0;
	int event_id			= 0;
	double p0				= 0.0;
	int phase				= 0;
	std::string id			= "";
	bool authorized			= false;
}state;

inline
void reset_state()
{
	state = State{};
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
	}
	catch (const std::exception &e)
	{
		printf("%s\n", e.what());
	}
}


double phase1(double p1, bool is_stable)
{
	if (is_stable)
	{
		store(p1);
	}
	return p1 - state.corr;
}


void phase0(double p1)
{
	if (state.p0 > p1 && state.p0 > state.min_weight) // reset on entering
	{
		reset_state();
	}
}


double fix(double p1, bool is_stable)
{
	double ret_value = p1;
	if (state.authorized)
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
	}
	else if (p1 >= state.min_weight)
	{
		printf("Unauthorized driver\n");
	}
	return ret_value;		
}

inline
void store_info(const std::string &com, const std::string &barcode)
{
	//															 event_id, com, barcode(event_id yet), (default timestamp)
	odbc::PreparedStatementRef ps = store_info_db->prepareStatement("INSERT INTO info VALUES(?, ?, ?)"\
																 "ON CONFLICT (event_id) DO UPDATE SET"
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
		ports[i].open();
		if (!ports[i].isOpen())
			printf("Error oppening port %s\n", ports[i].getPort().c_str());
	}

	try
	{
		SerialPool serial_pool{ std::move(ports) };
		while (true)
		{
			serial::Serial &serial_port = serial_pool.bad_wait();
			if (state.authorized)
			{
				printf("Error double authorization\n");
				// continue;
			}

			try
			{
				const size_t max_line_sz = 65536;
				std::string barcode = serial_port.readline(max_line_sz, "\r"); // from bar code
				barcode.pop_back(); // remove eol symbol

				if (!is_barcode_valid(barcode))
				{
					printf("Invalid barcode\n");
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
					
					state.authorized = true;
				}
			}
			catch (const std::exception &e)
			{
				printf("%s\n", e.what());
			}
		}
	}
	catch (const std::exception &e)
	{
		printf("%s\n", e.what());
		system("pause");
		exit(0);
	}
}


bool init_db()
{
	try
	{
		odbc_env = odbc::Environment::create();
		cars_db = odbc_env->createConnection();
		const char *cars_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=cars; UID=postgres; PWD=root;";
		cars_db->connect(cars_conn_str);

		store_db = odbc_env->createConnection();
		const char *store_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=store; UID=postgres; PWD=root;";
		store_db->connect(store_conn_str);

		store_info_db = odbc_env->createConnection();
		const char *store_info_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=store_info; UID=postgres; PWD=root;";
		store_info_db->connect(store_info_conn_str);
	}
	catch (const odbc::Exception &e)
	{
		printf("%s\n", e.what());
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


bool init_fixer(const char *ini_filename)
{
	if (!init_db())
	{
		return false;
	}

	reset_state();
	
	std::vector<PortInfo> pi;
	pi.push_back({ "COM4", 9600 });
	reader_thread = std::thread(serial_read, pi);
	reader_thread.detach();
	return true;
}
