#include "Retranslator.hpp"
#include "macro.hpp"
#include "../core/fixer.h"
#include "../core/Output.hpp"
#include <Control.hpp>
#include <Encode.hpp>
#include <Error.hpp>

#include <iostream>
#include <algorithm>
#include <string_view>
#include <chrono>

#include <atomic>

#include <Windows.h>

using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

static auto timePointLast = Time::now();
static ms timeSpanForStability = ms(500); // amount of time that weight shouldn't be changing to be stable
static double lastWeight = 0;
static int last_weight_not_in_delta_cnt = 0;
static int last_weight_not_in_delta_max_repeat = 10;
static double legit_weight_delta = 1000;

static size_t find_weight_response(const bytestring_view bsv)
{
	size_t pos = bsv.rfind('=');
	if (pos != bytestring_view::npos && bsv.size() - pos >= 9 - 1) // flag may be absent
	{
		// we have found '=' now we need to count how many number reserved bytes we have
		// if it's >=7 and all the chars there is numbers or ' ' | '$' | '%', , then return pos pointing on them
		// otherwise return npos
		constexpr std::string_view alphabet = " 0123456789$%";
		// +8 and not +9, bcs we care only about numbers, $ or % flag is not in our interest, though i leave them in alphabet
		for RANGE(pos + 1, pos + 7) {
			if (alphabet.find(static_cast<char>(bsv[i])) == std::string_view::npos)
				goto RETURN_NPOS;
		}
		return pos;
	}
	RETURN_NPOS:
	return std::string::npos;
}

inline double extract_weight(size_t ans_start_pos, const bytestring_view bsv)
{
	double res = 0;
	// skip first 2 syms "=(-| )", because vesysoft don't use them
	// i in [2, 7]
	// in vesysoft, if given "1 2 3" result = 123, so we ignore spaces too
	for	RANGE(ans_start_pos + 2, ans_start_pos + 8)	{
		if (isdigit(bsv[i])) {
			res = res * 10 + CHAR_TO_INT(bsv[i]); // bcs answer '33 $= 1345' is still valid we need to search circular
		}
	}
	return res;
}

// Weight values may be dirty so we need to decline them
// if prev_weight > || < curr_weight on 1000
// decline new weight
static void modify_resp(bytestring &bs)
{
	if (bs.size() >= 9)
	{ // when weight is returned, bs size == 9, if it's bigger there is some litter
		// valid weight response have the following scheme
		// =(-| )(\d| ){6}($|%)
		// last part, can be skipped
		size_t ans_start_pos = find_weight_response(bytestring_view{bs.data(), bs.size()});
		if (ans_start_pos != std::string::npos)
		{
			//printf("RESPONSE VALID AND STARTS FROM: %u\n", ans_start_pos);
			const double weight = extract_weight(ans_start_pos, bytestring_view{bs.data(), bs.size()});
			if (legit_weight_delta < weight - lastWeight) {
				last_weight_not_in_delta_cnt++;
				// this weight is probably invalid
				// pass if weight is not in delta for N times
				if (last_weight_not_in_delta_max_repeat > last_weight_not_in_delta_cnt) {
					return;
				}
			}
			last_weight_not_in_delta_cnt = 0;
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
			int fixed = static_cast<int>(fix(weight, stable));
			printf("Fixed: %i\n", fixed);

			// bcs weights use 6 bytes as with decimal in each, 2nd byte reserved for '-'
			if (fixed < 999999) {
				uint8_t flag = *bs.rbegin();
				bs.erase(std::end(bs) - 8, std::end(bs));

				std::vector<char> buf(8, '0');
				std::sprintf(buf.data(), "%0.6i", fixed);
				*buf.rbegin() = flag;

				// bcs sprintf inserts '\0' 
				if (fixed >= 0) *(buf.rbegin() + 1) = ' ';

				bs.insert(std::end(bs), std::cbegin(buf), std::cend(buf));
			}
			lastWeight = weight;
		}
	}
}

bool create_starter_proc(const std::string& current_dir) {
		// init corep
	HANDLE starter_h = Control::start_proc((current_dir + "starter.exe" + " controller_standalone.exe ").c_str());
	if (starter_h == INVALID_HANDLE_VALUE) // bcs i dont use this handle here
	{
		MessageBoxW(NULL, L"Неудалось запустить 'starter.exe'", L"ERROR", MB_OK);
		return false;
	}
	CloseHandle(starter_h);
	return true;
}

int main()
{
	// find current_dir
	char path_buf[MAX_PATH + 1] = {};
	GetModuleFileNameA(NULL, reinterpret_cast<char*>(path_buf), std::size(path_buf) - 1);	
	auto current_dir = std::string(path_buf);
	current_dir.erase(std::begin(current_dir) + current_dir.find_last_of('\\') + 1, std::end(current_dir));

	// load ini file
	constexpr auto ini_name = "ini.ini";
	const std::string path_to_ini = current_dir + ini_name;

	inipp::Ini<char> ini;
	FILE* fp = fopen(path_to_ini.c_str(), "rb");
	if (!fp) {
		dprintf(msg<12>());
		throw ctrl::error("Failed to open file %s\n", path_to_ini.c_str());
	}

	std::string ss;
	try
	{
		fdecode(ss, fp);
	}
	catch (std::exception& e)
	{
		fclose(fp);
		dprintf(msg<9>());
		printf("Failed to decode: \"%s\"", e.what());
	}
	fclose(fp);

	ini.parse(std::stringstream(ss));
	if (!create_starter_proc(current_dir)) {
		std::exit(1);
	}

	init_fixer(ini);

	// start retranslator
	try {
		const std::string src_port = ini.sections["DEFAULT"]["COM-end-for-retranslator"];
		const std::string dst_port = ini.sections["DEFAULT"]["COM-for-weights"];
		const std::string brdige = ini.sections["DEFAULT"]["COM-end-for-bridge"];
		const int ms_reading_timeout = std::stoi(ini.sections["DEFAULT"]["Weights-reading-timeout"]);
		timeSpanForStability = ms(std::stoi(ini.sections["DEFAULT"]["Time-span-for-stability-ms"]));
		legit_weight_delta = std::stod(ini.sections["DEFAULT"]["max-valid-weight-delta"]);
		
		auto r = Retranslator(src_port, dst_port, brdige);
		r.setModificator(modify_resp);
		r.start(ms_reading_timeout);
	}
	catch (const std::exception& e) {
		printf("%s\n", e.what());
	}
	return 0;
}