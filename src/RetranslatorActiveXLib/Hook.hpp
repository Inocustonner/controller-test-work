#pragma once
#include "include/HookTypes.hpp"

extern "C" {
	void fireEvent(EventType event);
	void setEventHook(EventType event, SetHook* onSet);
}

void initListener();
void startListener();
void stopListener();