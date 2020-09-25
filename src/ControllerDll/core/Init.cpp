#include "Init.hpp"
#include "State.hpp"
#include "Output.hpp"

#include <Error.hpp>
// #include <dllInjLib/dllInj.h>	// CreateConsole

#include <type_traits>
#include <optional>
#include <iostream>
#include <cstdlib>

#include <Windows.h>
using Section_Map = inipp::Ini<char>::Section;
using Sections_Map = inipp::Ini<char>::Sections;
static std::string get_module_dir() noexcept
{
	char path_buf[MAX_PATH + 1] = {};
	GetModuleFileNameA(NULL, reinterpret_cast<char*>(path_buf), std::size(path_buf) - 1);
	
	auto path = std::string(path_buf);
	path.erase(std::begin(path) + path.find_last_of('\\') + 1, std::end(path));
	return path;
}

template<typename KeyTM, typename KeyTt = KeyTM,
		 typename ValTM, typename ValTt = ValTM>
ValTM get(const std::map<KeyTM, ValTM>& map, KeyTt&& key, ValTt&& alt)
{
	if (auto it = map.find(key); it != std::cend(map))
		return it->second;
	else
		return alt;
}


// set if key present in map
template<typename KeyTM, typename KeyTt = KeyTM, typename ValTM, typename ValT>
void set_if(const std::map<KeyTM, ValTM>& map, KeyTt&& key, ValT& dest)
{
		if (auto it = map.find(key); it != std::cend(map))
			dest = it->second;
}

// invokes if key present in map
template<typename KeyTM, typename KeyTt = KeyTM, typename ValTM, typename FuncT>
void inv_if(const std::map<KeyTM, ValTM>& map, KeyTt&& key, FuncT func)
{
		if (auto it = map.find(key); it != std::cend(map))
			func(it);
}


static void debug_section(const Section_Map& debug_map) noexcept
{
	inv_if(debug_map, "LogLevel",
		[](auto& pair_str){ set_log_lvl(std::stoi(pair_str->second)); });
#if 0
	if (get_log_lvl() > 0)
		CreateConsole();
#endif
}


static void default_section(const Section_Map& default_map) noexcept
{
	inv_if(default_map, "reset_thr",
		[](auto& pair_str){ reset_thr = std::stoi(pair_str->second); });
	
	inv_if(default_map, "store_diff",
		[](auto& pair_str){ store_diff = std::stoi(pair_str->second); });

	inv_if(default_map, "min_weight",
		[](auto& pair_str) { default_min_weight = std::stoi(pair_str->second); });

	inv_if(default_map, "message-duration",
		[](auto& pair_str) { set_msg_duration(std::stoi(pair_str->second)); });
}

const void init_settings(const inipp::Ini<char>& ini)
{

	if (ini.sections.contains("DEBUG"))
	{
		debug_section(ini.sections.find("DEBUG")->second);
	}

	// to avoid writing to not yet connected debug db

	if (ini.sections.contains("DEFAULT"))
	{
		default_section(ini.sections.find("DEFAULT")->second);
	}
}