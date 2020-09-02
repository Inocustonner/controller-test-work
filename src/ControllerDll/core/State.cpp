#include "State.hpp"

static std::atomic<bool> auth_flag = false;

static double __reset_thr = 15000.0;
static double __store_diff = 200.0;
static double __default_min_weight = 10000.0;

const double& reset_thr = __reset_thr;			// externed
const double& store_diff = __store_diff;		// externed
const double& default_min_weight = __default_min_weight;

State state{};									// externed

void reset_state()
{
	state = State{};
	state.min_weight = reset_thr;
	auth_flag = false;
}


const std::atomic<bool>& authorized()
{
	return auth_flag;
}


void set_authorized()
{
	auth_flag = true;
}
