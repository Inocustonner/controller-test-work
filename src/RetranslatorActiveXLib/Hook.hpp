#pragma once
#include "include/HookTypes.hpp"
#include <functional>

extern "C" {
	void fireEvent(EventType event);
	void setEventHook(EventType event, void* onSet);
}

void initListener();
void startListener();
void stopListener();