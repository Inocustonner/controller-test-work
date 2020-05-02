#pragma once
#include <string>
#include <array>
#include <vector>

constexpr auto DB_CNT = 4; // cars, store, store_info, debug

struct DB_Auth
{
	std::string host, port, db, uid, pwd;
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
	Debug = 3
};

struct Settings
{
	std::vector<Port_Info> pi_v;
	std::array<DB_Auth, DB_CNT> dbi_a;
	std::string suffix = "";
};

bool init_databases(const std::array<DB_Auth, DB_CNT>& dbi_a);
const Settings init_settings();
