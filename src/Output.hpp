#pragma once
#include <odbc/Forwards.h>
#include "msg.hpp"

void set_log_lvl(const int lvl);
void set_log_db(odbc::ConnectionRef&& new_log_db);

void dprintf(const char *fmt, ...);
void dprintf(const msg_t msg);
