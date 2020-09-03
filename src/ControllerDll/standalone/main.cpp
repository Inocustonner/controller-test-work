#include "Retranslator.hpp"
#include "macro.hpp"
#include "../core/fixer.h"
#include <Control.hpp>

#include <iostream>
#include <algorithm>
#include <string_view>
#include <chrono>

#include <Windows.h>

using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

static auto timePointLast = Time::now();
static ms timeSpanForStability = ms(500); // amount of time that weight shouldn't be changing to be stable
static double lastWeight = 0;

static bytestring_view find_weight_response(const bytestring_view bsv)
{
	size_t pos = bsv.find_last_of('=', 0);
	if (pos != bytestring_view::npos)
	{
		// we have found '=' now we need to count how many number reserved bytes we have
		// if it's >=7 and all the chars there is numbers or ' ' | '$' | '%', , then return bsv pointing on them
		// otherwise return empty bsv
		if (bsv.size() - pos == 9)
		{
			constexpr std::string_view alphabet = " 0123456789$%";
			bool all = std::all_of(std::cbegin(bsv) + pos + 1, std::cend(bsv),
														 [alphabet](auto c) { return alphabet.find(static_cast<char>(c)) != bytestring_view::npos; });

			if (all)
			{
				return bytestring_view(bsv.data() + pos, bsv.size() - pos);
			}
		}
	}
	return bytestring_view{};
}

inline double extract_weight(const bytestring_view bsv)
{
	double res = 0;
	// skip first 2 syms "=(-| )", because vesysoft don't use them
	// i in [2, 7]
	// in vesysoft, if given "1 2 3" result = 123, so we ignore spaces too
	for
		RANGE(2, 8)
		{
			if (isdigit(bsv[i]))
			{
				res = res * 10 + CHAR_TO_INT(bsv[i]);
			}
		}
	return res;
}

static void modify_resp(bytestring &bs)
{
	if (bs.size() >= 9)
	{ // when weight is returned, bs size == 9, if it's bigger there is some litter
		// valid weight response have the following scheme
		// =(-| )(\d| ){6}($|%)
		// last part, can be skipped
		bytestring_view bsv = find_weight_response(bytestring_view{bs.data(), bs.size()});
		if (bsv.data())
		{
			printf("RESPONSE VALID: %.*s\n", bsv.size(), bsv.data());
			const double weight = extract_weight(bsv);
			bool stable = false;

			if (lastWeight == weight)
			{
				auto timeCurrentPoint = Time::now();
				if (timeCurrentPoint - timePointLast >= timeSpanForStability)
				{
					stable = true;
				}
			}
			else
			{
				timePointLast = Time::now();
			}
			printf("Extracted weight: %f[%s]\n", weight, stable ? "Stable" : "Unstable");
			printf("Fixed: %f", fix(weight, stable));
			lastWeight = weight;
		}
	}
}

int main()
{
	// find current_dir
	char path_buf[MAX_PATH + 1] = {};
	GetModuleFileNameA(NULL, reinterpret_cast<char*>(path_buf), std::size(path_buf) - 1);	
	auto current_dir = std::string(path_buf);
	current_dir.erase(std::begin(current_dir) + current_dir.find_last_of('\\') + 1, std::end(current_dir));

	// init corep
	HANDLE starter_h = Control::start_proc((current_dir + "starter.exe" + " controller_standalone.exe ").c_str());
	if (starter_h == INVALID_HANDLE_VALUE) // bcs i dont use this handle here
	{
		MessageBoxW(NULL, L"Неудалось запустить 'starter.exe'", L"ERROR", MB_OK);
		std::exit(1);
	}
	CloseHandle(starter_h);

	init_fixer("ini.ini");

	// start retranslator
	auto r = Retranslator("COM2", "COM1");
	r.setModificator(modify_resp);
	r.start();
	return 0;
}