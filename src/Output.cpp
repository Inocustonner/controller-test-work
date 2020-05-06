#include "Output.hpp"
#include "mmsg/mmsg.hpp"

// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Connection.h>
#include <odbc/Exception.h>
#include <odbc/PreparedStatement.h>

#include <tuple>
#include <array>
#include <cstdio>
#include <cstdarg>

static int log_lvl = default_log_lvl;
static odbc::ConnectionRef log_db;

int get_log_lvl()
{
	return log_lvl;
}


void set_log_lvl(const int lvl)
{
	log_lvl = lvl;
}


// void set_log_db(odbc::ConnectionRef new_log_db) noexcept
// {
// 	log_db = new_log_db;
// }


odbc::ConnectionRef& get_log_db()
{
	return log_db;
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
	try
	{
		auto ps = log_db->prepareStatement("INSERT INTO debug(code, message) VALUES(?, ?)");
			//"ON CONFLICT (id) DO UPDATE SET "
			//"id=EXCLUDED.id, code=EXCLUDED.code, message=EXCLUDED.message, ts=EXCLUDED.ts");
		ps->setInt(1, code);
		ps->setCString(2, str);
		ps->executeUpdate();
	}
	catch (const odbc::Exception &e)
	{
		if (log_lvl > 0)
			printf("Error while storing to debug db:\n\t%s", e.what());
		else
			mMsgBox(std::get<2>(msg<6>()), L"ERROR", 20000);					// throw message box
	}

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
