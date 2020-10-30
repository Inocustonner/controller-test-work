#include "Output.hpp"
#include "mmsg.hpp"

#include <tuple>
#include <array>
#include <cstdio>
#include <cstdarg>

static auto constexpr default_log_level = 2;

static int log_lvl = default_log_level;
static int msg_duration = 20000;

int get_log_lvl()
{
	return log_lvl;
}

int get_msg_duration() {
	return msg_duration;
}

void set_log_lvl(const int lvl)
{
	log_lvl = lvl;
}


void set_msg_duration(int secs)
{
	msg_duration = secs * 1000;
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


void dprintf(const msg_t msg)
{
	static int prev_code;
	// const char* str = make_buf("%s", std::get<1>);
	if (log_lvl > 0)
		fprintf(stderr, "%s\n", std::get<2>(msg));
	// if message is repeated or if last message != current message
	if (std::get<4>(msg) == true || std::get<0>(msg) != prev_code)
	{
		if (std::get<1>(msg) == MsgType::Error)
		{
			mMsgBox(L"Error", std::get<3>(msg), msg_duration);
		}
		else
		{
			mMsgBox(L"Warning", std::get<3>(msg), msg_duration);
		}
	}
	prev_code = std::get<0>(msg);
	// msgbox
}
