#include "State.hpp"
#include "Output.hpp"
#include "Databases.hpp"
#include "Init.hpp"

#include <cstdlib>

inline
double phase1(const double p1, const bool id_stable)
{
	double corr_weight = state.corr(p1);

	if (std::abs(p1 - state.p0) > store_diff)
	{
		store(state.com.c_str(), state.id.c_str(), state.event_id,
			static_cast<int>(corr_weight), static_cast<int>(p1));
	}
	return corr_weight;
}


inline
void phase0(const double p1)
{
	if (state.p0 > p1 && reset_thr > state.p0)
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
	init_settings();
	return true;
}
