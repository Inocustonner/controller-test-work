#pragma once
#include <inipp.h>

bool init_fixer(const inipp::Ini<char>& ini);

int fix(int value, bool is_stable);
