#include "shared.hpp"

enum class LightsEnum
{
	WAIT,
	ACCEPT,
	DENY
};

LightsEnum get_last_light_enum();
void light(LightsEnum l);
