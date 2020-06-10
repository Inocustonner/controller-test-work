#pragma once
#include <odbc/Forwards.h>
#include "msg.hpp"

constexpr int default_log_lvl = 2;
int get_log_lvl();
void set_log_lvl(const int lvl);
void set_msg_duration(int secs);

// void set_log_db(odbc::ConnectionRef new_log_db) noexcept;
odbc::ConnectionRef& get_log_db();

void dprintf(const char *fmt, ...);
void dprintf(const msg_t msg);
