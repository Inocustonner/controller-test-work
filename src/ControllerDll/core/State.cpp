#include "State.hpp"

static std::atomic<bool> auth_flag = false;

static comptype __reset_thr = 15000;
static comptype __store_diff = 200;
static comptype __default_min_weight = 10000;

const comptype& reset_thr = __reset_thr;			// externed
const comptype& store_diff = __store_diff;		// externed
const comptype& default_min_weight = __default_min_weight;

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
