#pragma once

#include <atomic>
#include <functional>
#include <string>

using comptype = int;

#define MAX_WEIGHT_DEFAULT 999999

struct State
{
	comptype max_weight = MAX_WEIGHT_DEFAULT;
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