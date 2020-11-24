#pragma once
#include <string>
#include <SLogger.hpp>
#include <functional>

void start_pipe_queue(bool& le, SLogger& lg);
void stop_pipe_queue();
void pipe_push_cmd(std::wstring cmd);
void pipe_set_timeout(unsigned long ms_timeout);
