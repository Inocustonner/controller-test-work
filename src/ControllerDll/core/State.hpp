#pragma once

#include <atomic>
#include <functional>
#include <string>

using comptype = int;

struct State
{
	comptype min_weight = 0; // defaults to reset_thr
	std::function<comptype(comptype)> corr;

	comptype p0 = 0;	// prev weight
	comptype p0s = 0; // prev stable weight

	bool authorized = false;
};

extern comptype reset_thr;
extern comptype default_min_weight;
extern State state;

void reset_state();