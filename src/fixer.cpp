#include "fixer.h"
#include <dllInjLib/dllInj.h>	// https://github.com/Inocustonner/dllInjectionLib

#include <serial/serial.h>		// https://github.com/wjwwood/serial

#include <odbc/Connection.h>	// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Environment.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>

#include <algorithm>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
// #include <iostream>

constexpr double input_error_default	= 10000.0;	// kg
constexpr double error_default			= 10000.0;	// kg
constexpr double min_weight_default		= 200.0;		// kg

odbc::EnvironmentRef odbc_env;
odbc::ConnectionRef cars_db, debug_db;
std::thread reader_thread;		// will be started in the init func

struct State
{
	double input_error;			// set once
	double error;					// set once

	double max;					// if curr_phase_i = 1
	double corr;					// if curr_phase_i = 1 recived from db
	double min_weight;
	double p0 = 0.0;				// previous weight value
	int curr_phase_i;				// number of current state. Possible values (0, 1, 2)
	int id;

	bool read_f;
};

State state = {};
void record(double input, double output)
{
	static unsigned long last_time = 0;
	// auto duration = std::chrono::system_clock::now().time_since_epoch();
	unsigned long curr_time = (unsigned long)time(NULL);
	
	if (curr_time == last_time) return; // no need in time duplications

	try
	{
		odbc::PreparedStatementRef ps = debug_db->prepareStatement("INSERT INTO debug VALUES(?, ?, ?, ?, ?, ?, ?)");
		ps->setULong(1, curr_time), ps->setInt(2, state.id), ps->setInt(3, (int)state.min_weight),
			ps->setInt(4, (int)state.corr), ps->setInt(5, (int)input), ps->setInt(6, (int)output),
			ps->setInt(7, state.curr_phase_i);
		ps->executeQuery();
		last_time = curr_time;
	}
	catch (std::exception &e)
	{
		printf("%s\n", e.what());
	}
}

// separate thread will wait for messages to come
// and when one has come thread checks whether someone is on the scales and if he is send beeep :D
void serial_reader(std::string portname,
					 uint32_t baudrate = 9600,
					 serial::bytesize_t bytesize = serial::bytesize_t::eightbits)
{
	printf("hello\n");
	// check if we can access the port
	try
	{
		serial::Serial serial_port(portname, baudrate, serial::Timeout(), bytesize);
		printf("Port is %s\n", serial_port.isOpen() ? "open" : "closed");
		serial_port.flush();
		// serial_port.setRTS(false);
		// serial_port.setDTR(false);
		while (true)
		{
			try
			{
				using namespace std::chrono_literals;
				if (!serial_port.available())
				{
					// serial_port.waitForChange();
					std::this_thread::sleep_for(1s);
					continue;
				}
				
				// if (state.curr_phase_i != 0)
				// {
				// 	printf("Error the last car still on scalars\n");
				// 	continue;
				// }
				
				const size_t line_maxsize = 65536;
				std::string id_str = serial_port.readline(line_maxsize, "\r");

				state.id = std::stoi(id_str);
				// get info about the car
				odbc::PreparedStatementRef pSelect = cars_db->prepareStatement("SELECT weight, corr FROM cars_table WHERE id = ?");
				pSelect->setInt(1, state.id);
				odbc::ResultSetRef rs = pSelect->executeQuery();
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
	catch (serial::PortNotOpenedException &e)
	{
		printf("%s\n", e.what());
		system("pause");
		exit(1);
	}

}


void reset_state()
{
	state.max = 0.0;
	state.corr = 0.0;
	state.min_weight = min_weight_default;
	state.id = 0;
}


double phase2(double p1)
{
	if (p1 > state.p0)
	{
		reset_state();
	}
	return p1;
}


double phase1(double p1, bool is_stable)
{
	if (is_stable)
		state.max = std::max(p1, state.max);
	return p1 - state.corr;
}


double phase0(double p1)
{
	// if (state.p0 > p1 && state.p0 > state.min_weight)		 // if entering phase0 from decreasing slope i.e. II point
	// {
	// 	reset_state();
	// }
	return p1;
}


double fix(double p1, bool is_stable)
{
	double ret_value = p1;
	if (p1 < state.min_weight)	// phase0 condition
	{
		state.curr_phase_i = 0;
		ret_value = phase0(p1);
	}
	else if (state.max - p1 <= state.error) // phase1 condition
	{
		state.curr_phase_i = 1;
		ret_value = phase1(p1, is_stable);
	}
	else // if (state.max - p1 > state.error) //phase 2 condition
	{
		state.curr_phase_i = 2;
		ret_value = phase2(p1);
	}
	state.p0 = p1;
	record(p1, ret_value);
	return ret_value;
}


bool init_dbs()
{
	try
	{
		odbc_env = odbc::Environment::create();

		cars_db = odbc_env->createConnection();
		const char *cars_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=cars; UID=postgres; PWD=root;";
		cars_db->connect(cars_conn_str);

		debug_db = odbc_env->createConnection();
		const char *debug_conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=debug; UID=postgres; PWD=root;";
		debug_db->connect(debug_conn_str);
	}
	catch (const odbc::Exception& e)
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


void read_opts(FILE *fp)
{
	if (!fscanf(fp, "input error: %lf", &state.input_error))
	{
		printf("No \"input error\" provided\nSetting it to default value: %lf\n", input_error_default);
		state.input_error = input_error_default;
	}
	if (!fscanf(fp, "error: %lf", &state.error))
	{
		printf("No \"error\" provided\nSetting it too default value: %lf\n", error_default);
		state.error = error_default;
	}
}


bool init_fixer(const char *ini_filename)
{
	if (!init_dbs())
		return false;

	std::string ini_path = path_to_ini(ini_filename);
	printf("settings file path : \n\t%s\n", ini_path.c_str());

	FILE *fp = fopen(ini_path.c_str(), "r");
	if (!fp)
	{
		// create settings file
	}
	else
	{
		read_opts(fp);
	}
	
	// fclose(fp);
	

	reset_state();
	// start reader thread
	reader_thread = std::thread(serial_reader, std::string("COM4"), 9600, serial::bytesize_t::eightbits);
	// serial_reader("COM4");
	reader_thread.detach();
	return true;
}
