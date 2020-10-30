#pragma once
#include "msg.hpp"

int get_log_lvl();
int get_msg_duration();
void set_log_lvl(const int lvl);
void set_msg_duration(int secs);

void dprintf(const char *fmt, ...);
void dprintf(const msg_t msg);
