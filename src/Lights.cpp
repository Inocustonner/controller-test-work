#include "Lights.hpp"
#include "Output.hpp"

#include <windows.h>

static std::string deny_cmd = "";
static std::string acc_cmd = "";
static std::string wait_cmd = "";

static LightsEnum last_light;

void set_light_deny(const std::string& deny)
{
	deny_cmd = deny;
}


void set_light_acc(const std::string& acc)
{
	acc_cmd = acc;
}


void set_light_wait(const std::string& wait)
{
	wait_cmd = wait;
}


LightsEnum get_last_light()
{
	return last_light;
}


void light(LightsEnum l)
{
	last_light = l;
	std::string_view cmd;
	switch (l)
	{
		case LightsEnum::Wait:
			cmd = wait_cmd;
			break;

		case LightsEnum::Acc:
			cmd = wait_cmd;
			break;

		case LightsEnum::Deny:
			cmd = deny_cmd;
			break;

		default:
			dprintf("Invalid LightsEnumerator value(%d)", static_cast<int>(l));
			return;
	}

	if (cmd == "")		// if no command given, don't do that
		return;

	STARTUPINFO si = { sizeof(si) };
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi = {};
	// std::cout << cmd << std::endl;
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
