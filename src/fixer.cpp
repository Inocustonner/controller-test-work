#include "fixer.h"
#include <dllInjLib/dllInj.h>	// https://github.com/Inocustonner/dllInjectionLib

#include <serial/serial.h>		// https://github.com/wjwwood/serial

#include <odbc/Connection.h>	// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Environment.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>

odbc::EnvironmentRef odbc_env;
odbc::ConnectionRef cars_db, store_db;
// in cars_db cars table is used
// in store_db info table is used
std::thread reader_thread;		// thread that reads from serial port


struct State
{
	double min_weight	= 200.0;
	double corr			= 5000.0;
	double p0			= 0.0;
	int phase			= 0;
	std::string id		= "";
}state;


void reset_state()
{
	state = State{};
}


void store(double p1)
{
	try
	{
		// id, /code id,/ mass, timestamp(default like)
		odbc::PreparedStatementRef pi =
			store_db->prepareStatement("INSERT INTO info VALUES(?, ?)");
		pi->setString(1, state.id);
		pi->setInt(2, (int)p1);
		pi->executeQuery();
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

	return ret_value;		
}


void serial_read(std::string portname,
				 uint32_t baudrate,
				 serial::bytesize_t bytesize)
{
	try
	{
		serial::Serial serial_port(portname, baudrate, serial::Timeout(), bytesize);

		while (true)
		{
			using namespace std::chrono_literals;
			if (!serial_port.available()) // maybe replace with conditional sleep
			{
				std::this_thread::sleep_for(1s);
				continue;
			}

			if (state.phase != 0)
			{
				printf("Error previous car is still on the scallars");
				// continue;
			}

			try
			{
				const size_t max_line_sz = 65536;
				state.id = serial_port.readline(max_line_sz, "\r");
				odbc::PreparedStatementRef ps = cars_db->prepareStatement("SELECT weight, corr FROM cars_table WHERE id = ?");
				ps->setString(1, state.id);
				odbc::ResultSetRef rs = ps->executeQuery();
				rs->next();

				state.min_weight = (double)*rs->getInt(1);
				state.corr = (double)*rs->getInt(2);
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
	reader_thread = std::thread(serial_read, std::string("COM4"), 9600, serial::bytesize_t::eightbits);
	reader_thread.detach();
	return true;
}
