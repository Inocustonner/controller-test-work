#include "State.hpp"
#include "Output.hpp"
#include "Databases.hpp"
#include "Init.hpp"
#include "Reader.hpp"
#include "Lights.hpp"

#include <Error.hpp>

#include <thread>
#include <cstdlib>

#include "fixer.h"

inline
comptype phase1(const comptype p1, const bool is_stable)
{
	comptype corr_weight = state.corr(p1);
	if (is_stable)
	{
		if (std::abs(p1 - state.p0s) > store_diff && is_stable)
		{
			store(state.com.c_str(), state.id.c_str(),
				static_cast<int>(corr_weight), static_cast<int>(p1));
		}
		state.p0s = p1;
	}
	return corr_weight;
}


inline
void phase0(const comptype p1)
{
	if (state.p0 > p1 
		&& state.p0 > reset_thr 
		&& reset_thr > p1)
	{
		dprintf("resetting\n");
		reset_state();
	}
}


comptype fix(const comptype p1, const bool is_stable)
{
	comptype ret_value = p1;
	if (authorized())
	{
		if (p1 < state.min_weight)
			phase0(p1);
		else
			ret_value = phase1(p1, is_stable);

	/*	if (get_last_light() == LightsEnum::Acc)
			light(LightsEnum::Wait);*/
	}
	else if (p1 > reset_thr)
	{
		dprintf(msg<1>());
	}
	state.p0 = p1;
	return ret_value;
}


bool init_fixer(const inipp::Ini<char>& ini)
{
	Settings setts = {};
	try
	{
		setts = init_settings(ini);
		init_databases(setts.db_provider, setts.dbi_a);
	}
	catch (const ctrl::error& e)
	{
		dprintf(e.what());
		if (get_log_lvl() > 1)
			std::system("pause");
		std::exit(1);
	}
	std::thread th(com_reader, setts.pi_v, setts.suffix, setts.udentified_car_allowed);
	th.detach();
	return true;
}
