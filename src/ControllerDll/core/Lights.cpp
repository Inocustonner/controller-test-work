#include "Lights.hpp"
#include "Output.hpp"

#include <windows.h>

#define CMD_PREFIX "start \"\" /min "

static std::string deny_cmd = "";
static std::string acc_cmd = "";
static std::string wait_cmd = "";

static LightsEnum last_light;

void set_light_deny(const std::string& deny)
{
	if (deny != "")
		deny_cmd = CMD_PREFIX + deny;
}


void set_light_acc(const std::string& acc)
{
	if (acc != "")
		acc_cmd = CMD_PREFIX + acc;
}


void set_light_wait(const std::string& wait)
{
	if (wait != "")
		wait_cmd = CMD_PREFIX + wait;
}


LightsEnum get_last_light()
{
	return last_light;
}


struct handle_data {
	unsigned long process_id;
	HWND window_handle;
};

BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle))
		return TRUE;
	data.window_handle = handle;
	return FALSE;
}

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.window_handle;
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
			cmd = acc_cmd;
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
	system(cmd.data());
}
