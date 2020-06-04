#include "Output.hpp"
#include "mmsg/mmsg.hpp"
#include <Control.hpp>

// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Connection.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>

#include <tuple>
#include <array>
#include <cstdio>
#include <cstdarg>

static int log_lvl = default_log_lvl;

int get_log_lvl()
{
	return log_lvl;
}


void set_log_lvl(const int lvl)
{
	log_lvl = lvl;
}

template<typename ...Args>
static const char* make_buf(const char *fmt, Args&& ...args) noexcept
{
	constexpr int max_buf_size = 1024;

	static std::array<char, max_buf_size> buf;
	buf.fill('\0');

	// check if this is va_list argument
	if (sizeof...(Args) == 1 &&
		std::is_same<std::decay_t<Args...>, va_list>::value)
	{
		vsnprintf(buf.data(), std::size(buf) - 1, fmt, args...);
	}
	else
	{
		snprintf(buf.data(), std::size(buf) - 1, fmt, args...);
	}
	return buf.data();
}


static void dblog(int code, const char* str) noexcept
{
	std::lock_guard<std::mutex> guard(Control::get_main_mutex());

	//Control::lockMutexDebug();
	command_s* cmd_p = Control::get_command();
	cmd_p->cmd = Cmd::Store_Debug;

	data_s* data_p = Control::next_data(nullptr);
	data_p->type = DataType::Int;
	data_p->size = sizeof(int);
	*reinterpret_cast<int*>(data_p->body()) = code;

	data_p = Control::next_data(data_p);
	data_p->type = DataType::Str;
	data_p->size = std::strlen(str) + 1;
	std::memcpy(data_p->body(), str, data_p->size);

	//Control::releaseMutexDebug();
	Control::SetEventMain();
	Control::syncDb();
}


void dprintf(const char *fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);

	const char* str = make_buf(fmt, vl);
	if (log_lvl > 0)
		fprintf(stderr, "%s", str);
	if (log_lvl < 2)
		dblog(-1, str);

	va_end(vl);
}


void dprintf(const msg_t msg)
{
	static int prev_code;
	// const char* str = make_buf("%s", std::get<1>);
	if (log_lvl > 0)
		fprintf(stderr, "%s\n", std::get<1>(msg));
	if (std::get<0>(msg) != prev_code)
	{
		if (log_lvl < 2) // avoid multiple writing to db
		{
			dblog(std::get<0>(msg), std::get<1>(msg));
		}
		mMsgBox(std::get<2>(msg), L"ERROR", 20000);
	}
	prev_code = std::get<0>(msg);
	// msgbox
}
