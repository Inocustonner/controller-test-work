#pragma once

#include <atomic>
#include <functional>
#include <string>

struct State
{
	double min_weight	= 0.0;	// defaults to reset_thr
	std::function<double(double)> corr;

	int event_id		= 0;

	double p0			= 0; // prev weight
	double p0s			= 0; // prev stable weight
	// int phase			= 0;
	std::string id	= "";		// for dbs
	std::string com	= "";		// for dbs
};


extern const double& reset_thr;
extern const double& store_diff;
extern const double& default_min_weight;
extern State state;

void reset_state();

[[nodiscard]]
const std::atomic<bool>& authorized();

void set_authorized();

inline
void set_reset_thr(double reset_thr_value)
{
	const_cast<double&>(reset_thr) = reset_thr_value;
}

inline
void set_store_diff(double store_diff_value)
{
	const_cast<double&>(store_diff) = store_diff_value;
}

inline
void set_default_min_weight(double weight)
{
	const_cast<double&>(default_min_weight) = weight;
}
