#pragma once
#include <string>
#include <array>
#include <vector>

struct DB_Auth
{
	std::string conn_str;
};

enum class parity_t
{
	parity_none		= 0,
	parity_odd		= 1,
	parity_even		= 2	
};


struct Port_Info
{
	std::string name;
	int baudrate		= 9600;
	int byte_size		= 8;				// 5, 6, 7, 8
	parity_t parity	= parity_t::parity_none;	
};

enum class DBEnum
{
	Cars = 0,
	Store = 1,
	Store_Info = 2,
	Debug = 3,
	Drivers = 4,
};
constexpr auto DB_CNT = 5; // cars, store, store_info, debug


struct Settings
{
	std::vector<Port_Info> pi_v;
	std::array<DB_Auth, DB_CNT> dbi_a;
	std::string suffix = "";
};

const Settings init_settings();
void init_databases(const std::array<DB_Auth, DB_CNT>& dbi_a);
