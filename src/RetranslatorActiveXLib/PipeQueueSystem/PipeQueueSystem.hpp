#pragma once
#include <string>

void start_pipe_queue();
void stop_pipe_queue();
void pipe_push_cmd(std::wstring cmd);
void pipe_set_timeout(unsigned long ms_timeout);
