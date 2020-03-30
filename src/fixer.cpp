#include "fixer.h"
#include <chrono>
#include <dllInjLib/dllInj.h>
#include <odbc/odbc.hpp>
#include <algorithm>
#include <string>

// #include <iostream>

constexpr double input_error_default	= 10000.0;	// kg
constexpr double error_default			= 10000.0;	// kg

Odbc *debug_db_p, *cars_db_p, *those_db_p;

struct State
{
	double input_error;			// set once
	double error;				// set once

	double max = 0.0;			// if curr_phase_i = 1
	double corr = 0.0;			// if curr_phase_i = 1 recived from db
	double min_weight = 0.0;	// weight value after what start phase 1
	double p0 = 0.0;			// previous weight value
	int curr_phase_i;			// number of current state. Possible values (0, 1, 2)

	bool read_f;
};

State state = {};


// void record(double p1, double ret_val)
// {	
// 	long long unsigned time = std::chrono::duration_cast<std::chrono::milliseconds>(
// 																					  std::chrono::system_clock::now().time_since_epoch()
// 																					  ).count();
// 	static decltype(time) prev_time = 0;
	
// 	if (time != prev_time)
// 	{
// 		int inp_value = (int)p1;
// 		int res_value = (int)ret_val;
// 		char query_buffer[128] = {};
// 		snprintf(query_buffer, std::size(query_buffer), "INSERT INTO debug_phases "
// 				 "VALUES(%llu, %d, %d, %d, %d, %d, %d, %d);", time, inp_value, res_value, state.curr_phase_i, (int)state.max, (int)state.corr, (int)state.min_weight, (int)state.error);
// 		Stmt *stmt = debug_db_p->exec_query(query_buffer);
// 		if (!stmt)
// 		{
// 			printf("error : %d\n", stmt->get_status_code());
// 		}	
// 	}
// 	prev_time = time;
// }


void update_state()
{
	// retreview last id
	Stmt *stmt = those_db_p->exec_query("SELECT id FROM fac_cars ORDER BY id DESC LIMIT 1;");
	if (stmt == nullptr)
	{
		printf("Error %d\n", those_db_p->get_status_code());
		return;
	}

	Data_Matrix_t id_v = stmt->get_all_rows();
	if (std::size(id_v) == 0)
	{
		printf("Error %d\n", stmt->get_status_code());
		return;
	}
	
	int id = *(int*)id_v[0][0].get_data();

	// get info about the car
	char query_buffer[128] = {};
	snprintf(query_buffer, std::size(query_buffer),
			 "SELECT %s %s FROM cars_table WHERE id=%d;", "weight", "correct", id);
	stmt = cars_db_p->exec_query(reinterpret_cast<const char*>(query_buffer));
	if (stmt == nullptr)
	{
		printf("Error %d\n", those_db_p->get_status_code());
		return;
	}

	Data_Matrix_t info_v = stmt->get_all_rows();
	if (std::size(info_v) == 0)
	{
		printf("Error %d\n", stmt->get_status_code());
		return;
	}

	state.min_weight = (double)*(int*)(info_v[0][0].get_data());
	state.corr = (double)*(int*)(info_v[0][1].get_data());
}


void reset_state()
{
	state.max = 0.0;
	state.corr = 0.0;
	state.read_f = false;
}


double phase2(double p1)
{
	if (p1 > state.p0)
	{
		reset_state();
		state.read_f = true;
	}
	return p1;
}

double phase1(double p1, bool is_stable)
{
	if (state.read_f && state.p0 <= state.min_weight) // I point
	{
		// read from db
		state.read_f = false;
	}
	if (is_stable)
		state.max = std::max(p1, state.max);
	return p1 - state.corr;
}


double phase0(double p1)
{
	if (p1 >= state.p0)
	{
		state.read_f = true;
	}
	else if (state.p0 > p1 && state.p0 > state.min_weight)		 // if entering phase0 from decreasing slope i.e. II point
	{
		reset_state();
	}
	return p1;
}


double fix(double p1, bool is_stable)
{
	printf("%d\n", (int)is_stable);
	double ret_value = p1;							 // = p1 is for debug purpose
	if (p1 < state.min_weight)
	{
		state.curr_phase_i = 0;
		ret_value = phase0(p1);
	}
	else if (state.max - p1 <= state.error)		// if before III point
	{
		state.curr_phase_i = 1;
		ret_value = phase1(p1, is_stable);
	}
	else // if (state.max - p1 > state.error) 
	{
		state.curr_phase_i = 2;
		ret_value = phase2(p1);
	}
	state.p0 = p1;
	// record(p1, ret_value);
	return ret_value;
}


bool init_dbs()
{
	const char *conn_str = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=cars; UID=postgres; PWD=root;";
	cars_db_p = new Odbc;
	cars_db_p->set_connection_string(conn_str);
	
	const char *conn_str2 = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=those_cars; UID=postgres; PWD=root;";
	those_db_p = new Odbc;
	those_db_p->set_connection_string(conn_str2);

	const char *conn_str3 = "DRIVER={PostgreSQL ANSI}; SERVER=localhost; PORT=5432; DATABASE=debug; UID=postgres; PWD=root;";
	debug_db_p = new Odbc;
	debug_db_p->set_connection_string(conn_str3);

	return cars_db_p->connect() && those_db_p->connect() && debug_db_p->connect();
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
	if (!fp || !fscanf(fp, "input error: %lf", &state.input_error))
	{
		printf("No \"input error\" provided\nSetting it to default value: %lf\n", input_error_default);
		state.input_error = input_error_default;
	}
	if (!fp || !fscanf(fp, "error: %lf", &state.error))
	{
		printf("No \"error\" provided\nSetting it too default value: %lf\n", error_default);
		state.error = error_default;
	}
}


bool init_fixer(const char *ini_filename)
{
	if (!init_env() || !init_dbs())
		return false;

	std::string ini_path = path_to_ini(ini_filename);
	printf("settings file path : \n\t%s\n", ini_path.c_str());

	FILE *fp = fopen(ini_path.c_str(), "r");
	read_opts(fp);
	if (fp)
		fclose(fp);

	reset_state();

	return true;
}


void close_connection()
{
	delete cars_db_p;
	delete debug_db_p;
	delete those_db_p;
	free_env();
}
