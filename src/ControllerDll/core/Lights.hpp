#include <string>

enum class LightsEnum
{
	Wait,
	Acc,
	Deny
};

void set_light_deny(const std::string& deny);
void set_light_acc(const std::string& acc);
void set_light_wait(const std::string& wait);

LightsEnum get_last_light();

void light(LightsEnum l);
