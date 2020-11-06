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

	comptype reset_thr;

	bool authorized = false;
	bool passed_upper_gate = false;
};

extern comptype reset_thr;
extern comptype default_min_weight;
extern State state;

extern comptype rounding;
static_assert(std::is_same_v<comptype, int>, "rounding only working for integer types");

extern double reset_thr_koef;
void reset_state();