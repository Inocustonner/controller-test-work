#pragma once

#include <tuple>
#include <array>

using msg_t = std::tuple<int, const char*, const wchar_t*>;

constexpr msg_t _msg(int code);

#define MSG(CODE, ENG, RUS) msg_t(CODE, ENG, L##RUS)	// to add msg and code
constexpr std::array code_msg =
{
	// -1 is for don't standart messages
	MSG(0, "Error: double authorization\n",		"Ошибка двойная авторизация\n"),
	MSG(1, "Error: Unauthorized driver\n",		"Ошибка водитель не авторизован\n"),
	MSG(2, "Error: Invalid barcode\n",			"Ошибка не верный формат штрихкода\n"),
	MSG(3, "Com port connecting error\n",		"ошибка подключения com port'а\n"),
	MSG(4, "Unknown option ignored\n",			"Ошибка неизвестная опция проигнорирована\n"),
	MSG(5, "Error while writing to a db\n",		"Ошибка во время записи в базу данных\n"),
	MSG(6, "Error while reading from a db\n",	"Ошибка во время обращения к базе данных\n")
};


template<int code>
constexpr msg_t msg(void)
{
	static_assert(code >= std::get<0>(*std::cbegin(code_msg))
				&& code <= std::get<0>(*std::crbegin(code_msg)), "Message code is out of bounds");
	// std::find don't work for some reason
	for (const auto tup : code_msg)
		if (std::get<0>(tup) == code) return tup;
}
