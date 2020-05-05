#include "Init.hpp"
#include "State.hpp"
#include "Output.hpp"

#include <inipp.h>
#include <dllInjLib/dllInj.h>	// CreateConsole

#include <type_traits>
#include <optional>
#include <iostream>
#include <fstream>

#include <Windows.h>
using Section_Map = inipp::Ini<char>::Section;
using Sections_Map = inipp::Ini<char>::Sections;
static std::string get_module_dir() noexcept
{
	char path_buf[MAX_PATH + 1] = {};
	GetModuleFileName(NULL, reinterpret_cast<char*>(path_buf), std::size(path_buf) - 1);
	
	auto path = std::string(path_buf);
	path.erase(std::begin(path) + path.find_last_of('\\') + 1, std::end(path));
	return path;
}

template<typename KeyTM, typename KeyTt = KeyTM,
		 typename ValTM, typename ValTt = ValTM>
ValTM get(std::map<KeyTM, ValTM>& map, KeyTt&& key, ValTt&& alt)
{
	if (auto it = map.find(key); it != std::end(map))
		return it->second;
	else
		return alt;
}


// set if key present in map
template<typename KeyTM, typename KeyTt = KeyTM, typename ValTM, typename ValT>
void set_if(std::map<KeyTM, ValTM>& map, KeyTt&& key, ValT& dest)
{
		if (auto it = map.find(key); it != std::end(map))
			dest = it->second;
}

// invokes if key present in map
template<typename KeyTM, typename KeyTt = KeyTM, typename ValTM, typename FuncT>
void inv_if(std::map<KeyTM, ValTM>& map, KeyTt&& key, FuncT func)
{
		if (auto it = map.find(key); it != std::end(map))
			func(it);
}


static void debug_section(Section_Map& debug_map) noexcept
{
	inv_if(debug_map, "LogLevel",
		[](auto& pair_str){ set_log_lvl(std::stoi(pair_str->second)); });
	if (get_log_lvl > 0)
		CreateConsole();
}


static void default_section(Section_Map& default_map, Settings& setts) noexcept
{
	inv_if(default_map, "reset_thr",
		[](auto& pair_str){ set_reset_thr(std::stod(pair_str->second)); });
	
	inv_if(default_map, "store_diff",
		[](auto& pair_str){ set_store_diff(std::stod(pair_str->second)); });
	
	set_if(default_map, "suffix", setts.suffix);

	if (setts.suffix == "")
		dprintf("No suffix set");
	else if (setts.suffix == "CR")
		setts.suffix = "\r";
	else if (setts.suffix == "LF")
		setts.suffix = "\n";
	else if (setts.suffix == "CRLF")
		setts.suffix = "\r\n";
	else if (setts.suffix == "LFCR")
		setts.suffix = "\n\r";
	else
		dprintf("Custom suffix:\n\t%s", setts.suffix.c_str());

	std::istringstream iss(get(default_map, "carsdb", ""));
	DB_Auth& cars = setts.dbi_a[static_cast<int>(DBEnum::Cars)];
	iss >> cars.host >> cars.port
		>> cars.db >> cars.uid >> cars.pwd;

	iss = std::istringstream(get(default_map, "storedb", ""));
	DB_Auth& store = setts.dbi_a[static_cast<int>(DBEnum::Store)];
	iss >> store.host >> store.port
		>> store.db >> store.uid >> store.pwd;

	iss = std::istringstream(get(default_map, "store_infodb", ""));
	DB_Auth& store_info = setts.dbi_a[static_cast<int>(DBEnum::Store_Info)];
	iss >> store_info.host >> store_info.port
		>> store_info.db >> store_info.uid >> store_info.pwd;

	iss = std::istringstream(get(default_map, "debugdb", ""));
	DB_Auth& debug = setts.dbi_a[static_cast<int>(DBEnum::Store_Info)];
	iss >> debug.host >> debug.port
		>> debug.db >> debug.uid >> debug.pwd;	
}

static void com_section(Section_Map& com_map, Settings& setts)
{
	for (auto it = std::cbegin(com_map); it != std::cend(com_map); ++it)
	{
		Port_Info pi = { it->first };
		std::istringstream is(it->second);
		std::string parity;
		is >> pi.baudrate >> pi.byte_size >>parity;

		if (pi.byte_size < 5 || pi.byte_size > 8)
			dprintf("Invalid byte size %d. Byte size set to %d", pi.byte_size, pi.byte_size = 8);

		if (parity == "even")
			pi.parity = parity_t::parity_even;
		else if (parity == "odd")
			pi.parity = parity_t::parity_odd;
		else if (parity == "none" || parity == "")
			pi.parity = parity_t::parity_none;
		else
			dprintf("Unknown parity. Parity set to 'none'.\n");
		setts.pi_v.push_back(pi);
	}
}

const Settings init_settings()
{
	constexpr auto ini_name = "ini.ini";
	const std::string path_to_ini = get_module_dir() + ini_name;
	CreateConsole();

	dprintf("ini path:\n\t%s\n", path_to_ini.c_str());
	
	// constexpr std::array ini_sections = { "DEFAULT", "COM", "DEBUG" };
	Settings setts = {};
	inipp::Ini<char> ini;
	std::ifstream is(path_to_ini);
	ini.parse(is);

	if (ini.sections.contains("DEBUG"))
	{
		debug_section(ini.sections.find("DEBUG")->second);
	}

	if (ini.sections.contains("DEFAULT"))
	{
		default_section(ini.sections.find("DEFAULT")->second, setts);
	}

	if (ini.sections.contains("COM"))
	{
		com_section(ini.sections.find("COM")->second, setts);
	}
	return {};
}
