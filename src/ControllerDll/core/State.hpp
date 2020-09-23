#pragma once

#include <atomic>
#include <functional>
#include <string>

using comptype = int;

struct State
{
	comptype min_weight	= 0;	// defaults to reset_thr
	std::function<comptype(comptype)> corr;

	int event_id		= 0;

	comptype p0			= 0; // prev weight
	comptype p0s			= 0; // prev stable weight
	// int phase			= 0;
	std::string id	= "";		// for dbs
	std::string com	= "";		// for dbs
};


extern const comptype& reset_thr;
extern const comptype& store_diff;
extern const comptype& default_min_weight;
extern State state;

void reset_state();

[[nodiscard]]
const std::atomic<bool>& authorized();

void set_authorized();

inline
void set_reset_thr(comptype reset_thr_value)
{
	const_cast<comptype&>(reset_thr) = reset_thr_value;
}

inline
void set_store_diff(comptype store_diff_value)
{
	const_cast<comptype&>(store_diff) = store_diff_value;
}

inline
void set_default_min_weight(comptype weight)
{
	const_cast<comptype&>(default_min_weight) = weight;
}
