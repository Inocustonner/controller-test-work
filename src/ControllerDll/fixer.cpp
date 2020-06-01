#include "State.hpp"
#include "Output.hpp"
#include "Databases.hpp"
#include "Init.hpp"
#include "Reader.hpp"
#include "Lights.hpp"

#include <thread>
#include <cstdlib>

inline
double phase1(const double p1, const bool id_stable)
{
	double corr_weight = state.corr(p1);

	if (std::abs(p1 - state.p0) > store_diff)
	{
		store(state.com.c_str(), state.id.c_str(),
			static_cast<int>(corr_weight), static_cast<int>(p1));
	}
	return corr_weight;
}


inline
void phase0(const double p1)
{
	if (state.p0 > p1 
		&& state.p0 > reset_thr 
		&& reset_thr > p1)
	{
		dprintf("resetting\n");
		reset_state();
	}
}


double fix(const double p1, const bool is_stable)
{
	double ret_value = p1;
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


bool init_fixer(const char *ini_filename)
{
	const Settings setts = init_settings();
	init_databases(setts.dbi_a);
	std::thread th(com_reader, setts.pi_v, setts.suffix);
	th.detach();
	return true;
}
