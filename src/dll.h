#pragma once
#define EXPORT(type, name) extern "C" type __declspec(dllexport) name
EXPORT(void, init)();
