#pragma once
#include "bytestring.hpp"
#include <vector>
#include <string_view>
/* Writes set of commands to dst_commands_unique and returns cnt written. max_size - maximum size of dst_commands_uniq */
int parse_commands_unique(const bytestring &bs, uint8_t* dst_commands_uniq, int max_size);