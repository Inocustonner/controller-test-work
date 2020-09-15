#pragma once
#include <inipp.h>

bool init_fixer(const inipp::Ini<char>& ini);

double fix(double value, bool is_stable);
