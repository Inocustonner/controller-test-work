#include "lights.hpp"

#include <string_view>
#include <iostream>
#include <windows.h>

std::string lights_wait_cmd;
std::string lights_acc_cmd;
std::string lights_deny_cmd;

static LightsEnum last_state;

LightsEnum get_last_light_enum()
{
	return last_state;
}

void light(LightsEnum l)
{
	last_state = l;
	std::string_view cmd;
	switch (l)
	{
		case LightsEnum::WAIT:
			cmd = lights_wait_cmd;
			break;

		case LightsEnum::ACCEPT:
			cmd = lights_wait_cmd;
			break;

		case LightsEnum::DENY:
			cmd = lights_wait_cmd;
			break;

		default:
			dprintf("Invalid LightsEnumerator value(%d)", static_cast<int>(l));
			return;
	}

	STARTUPINFO si = { sizeof(si) };
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi = {};
	std::cout << cmd << std::endl;
	if (!CreateProcessA(NULL,
						const_cast<char*>(cmd.data()),
						NULL,
						NULL,
						FALSE,
						0,
						NULL,
						NULL,
						&si,
						&pi))
	{
		printf("%d: CreateProcess failed\n", GetLastError());
		return;
	}

	// CLOSE HANDLES AAAAAAAAAA!!!!!!!!!!!!!
}
