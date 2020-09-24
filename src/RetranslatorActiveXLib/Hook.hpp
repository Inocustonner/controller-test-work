#pragma once
#include "include/HookTypes.hpp"

void fireEvent(EventType event);

extern "C"
void setEventHook(EventType event, SetHook *onSet);

void initListener();
void startListener();
void stopListener();