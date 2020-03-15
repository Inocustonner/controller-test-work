#include "fixer.h"
#include "dllInj.h"
#include <string>
struct Opts
{
	FILE* ini_fp;
	double urb, lrb;							// upper reading bound, lower reading bound
	double fix_upper_thr, fix_lower_thr;	// fixation upper / lower threshold
	double adj_value;
	bool read;								// has we read already new values?
};


Opts adj_opts = {};


bool read_opts()
{
	fseek(adj_opts.ini_fp, 0, SEEK_SET);
	if (!fscanf(adj_opts.ini_fp, "upper reading bound : %lf\n", &adj_opts.urb))
	{
		printf("upper reading bound option is unspecified\n");
		return false;
	}
	if(!fscanf(adj_opts.ini_fp, "lower reading bound : %lf\n", &adj_opts.lrb))
	{
		printf("lower reading bound option is unspecified\n");
		return false;
	}
	if(!fscanf(adj_opts.ini_fp, "fixing upper thr : %lf\n", &adj_opts.fix_upper_thr))
	{
		printf("fixing upper thr option is unspecified\n");
		return false;
	}
	if(!fscanf(adj_opts.ini_fp, "fixing lower thr : %lf\n", &adj_opts.fix_lower_thr))
	{
		printf("fixing lower thr option is unspecified\n");
		return false;
	}
	if(!fscanf(adj_opts.ini_fp, "adj value : %lf\n", &adj_opts.adj_value))
	{
		printf("adj value option is unspecified\n");
		return false;
	}
	// printf("%lf\n", adj_opts.adj_value);
	return true;
}


double fix(double value)
{
	// read_opts();
	adj_opts.read = false;
	
	if (value > adj_opts.fix_upper_thr || value < adj_opts.fix_lower_thr)
	{
		value += adj_opts.adj_value;
	}
	else if (adj_opts.urb>= value && value <= adj_opts.lrb)
	{
		if (!adj_opts.read)
			adj_opts.read = read_opts();
		else
			adj_opts.read = true; // we've read data last time and still within reading bounds
	}

	return value;
}


std::string path_to_ini(std::string ini_filename)
{
	std::string path_to_ini = GetCurrentProcPath();
	size_t path_len = path_to_ini.find_last_of("\\") + 1;
	path_to_ini.replace(path_len, path_to_ini.size() - path_len, ini_filename);
	return path_to_ini;
}


bool init_fixer(const char *ini_filename)
{
	std::string ini_path = path_to_ini(ini_filename);
	printf("ini : %s\n", ini_path.c_str());
	adj_opts.ini_fp = fopen(ini_path.c_str(), "r");
	if (adj_opts.ini_fp)
	{
		return read_opts();
	}
	else
	{
		printf("Failed to open %s\n", ini_filename);
		return false;
	}
}
