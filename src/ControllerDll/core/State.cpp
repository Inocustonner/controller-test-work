#include "State.hpp"

comptype reset_thr = 15000;
comptype default_min_weight = 10000;

State state{};									// externed

void reset_state()
{
	state.p0 = 0;
	state.p0s = 0;
	state.min_weight = reset_thr;
	state.authorized = false;
}