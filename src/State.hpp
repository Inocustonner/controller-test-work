#pragma once

#include <atomic>
#include <functional>
#include <string>

struct State
{
	double min_weight	= 0.0;	// defaults to reset_thr
	std::function<double(double)> corr;

	int event_id		= 0;

	double p0			= 0;
	// int phase			= 0;
	std::string id	= "";		// for dbs
	std::string com	= "";		// for dbs
};


extern const double& reset_thr;
extern const double& store_diff;

extern State state;

State& get_state();
void reset_state();

[[nodiscard]]
const std::atomic<bool>& authorized();

void set_authorized();

inline
void set_reset_thr(const double reset_thr_value)
{
	const_cast<double&>(reset_thr) = reset_thr_value;
}

inline
void set_store_diff(const double store_diff_value)
{
	const_cast<double&>(store_diff) = store_diff_value;
}
