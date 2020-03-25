#include "fixer.h"
#include <dllInjLib/dllInj.h>
#include <algorithm>
#include <string>

// #include <iostream>

constexpr double phase0_thr_default = 10000.0; // kg
constexpr double error_default = 100.0;		// kg

struct State
{
	double phase0_thr;	  	// set once
	double error;				// set once

	double max = 0.0;			// if curr_phase_i = 1
	double corr = 0.0;		// if curr_phase_i = 1 recived from db

	double p0 = 0.0;			// previous weight value
	int curr_phase_i;			// number of current state. Possible values (0, 1, 2)

	bool read_f;
};

State state = {};

void reset_state()
{
	state.max = 0.0;
	state.corr = 0.0;
	state.read_f = false;
}


double phase2(double p1)
{
	printf("phase 2\n");
	if (p1 > state.p0)
	{
		reset_state();
		state.read_f = true;
	}
	return p1;
}

// double phase1(double p1, bool is_stable)
double phase1(double p1)
{
	printf("phase 1\n");
	if (state.read_f && state.p0 <= state.phase0_thr) // I point
	{
		// read from db
		state.read_f = false;
	}
	state.max = std::max(p1, state.max);
	// if (is_stable) state.max = std::max(p1, max);
	return p1 - state.corr;
}


double phase0(double p1)
{
	printf("phase 0\n");
	if (p1 >= state.p0)
	{
		state.read_f = true;
	}
	else if (state.p0 > p1 && state.p0 > state.phase0_thr)		 // if entering phase0 from decreasing slope i.e. II point
	{
		reset_state();
	}
	return p1;
}


double fix(double p1)
{
	double ret_value = p1;							 // = p1 is for debug purpose
	if (p1 < state.phase0_thr)
	{
		state.curr_phase_i = 0;
		ret_value = phase0(p1);
	}
	else if (state.max - p1 <= state.error)		// if before III point
	{
		state.curr_phase_i = 1;
		ret_value = phase1(p1);
	}
	else // if (state.max - p1 > state.error) 
	{
		state.curr_phase_i = 2;
		ret_value = phase2(p1);
	}
	state.p0 = p1;
	return ret_value;
}


void read_opts(FILE *fp)
{
	if (!fp || !fscanf(fp, "phase0 thr: %lf", &state.phase0_thr))
	{
		printf("No \"phase0 thr\" provided\nSetting it to default value: %lf\n", phase0_thr_default);
		state.phase0_thr = phase0_thr_default;
	}
	if (!fp || !fscanf(fp, "error: %lf", &state.error))
	{
		printf("No \"error\" provided\nSetting it too default value: %lf\n", error_default);
		state.error = error_default;
	}
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
	std::string ini_path = path_to_ini(ini_filename);
	printf("settings file path : \n\t%s\n", ini_path.c_str());

	FILE *fp = fopen(ini_path.c_str(), "r");
	read_opts(fp);
	if (fp)
		fclose(fp);

	reset_state();

	return true;
}
