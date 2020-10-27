#pragma once

#include <tuple>
#include <array>

enum class MsgType
{
	Warning,
	Error
};

using msg_t = std::tuple<int, MsgType, const char*, const wchar_t*, bool>;
#define MSG(CODE, TYPE, ENG, RUS) msg_t(CODE, TYPE, ENG, L## #RUS, true)	// to add msg and code
#define NOREP_MSG(CODE, TYPE, ENG, RUS) msg_t(CODE, TYPE, ENG, L## #RUS, false)	// to add unrepeated msg and code 

constexpr std::array code_msg =
{
	// -1 is for custom messages
	MSG(0, MsgType::Warning, "Error: Invalid timeSpanForStability, must be in {0..10 000}\nsetting to default", "Неверное значение timeSpanForStability \nдолжно быть в промежутке {0..10 000}\nустановка стандартного значения"),
	MSG(1, MsgType::Error, "Error: Couldn't open ini.ini", "Не смог открыть ini.ini файл настроек"),
	MSG(2, MsgType::Error, "Error: Couldn't decode ini.ini", "Не смог расшифровать ini.ini файл настроек"),
	MSG(3, MsgType::Error, "Error: In Retranslator", "Ошибка в Retranslator, \nвключите консоль чтобы увидеть подробности\n")
};

#pragma warning(push)
#pragma warning(disable : 4715)

template<int code>
constexpr msg_t msg(void)
{
	static_assert(code >= std::get<0>(*std::cbegin(code_msg))
		&& code <= std::get<0>(*std::crbegin(code_msg)), "Message code is out of bounds");
	// std::find don't work for some reason
	for (const auto tup : code_msg)
		if (std::get<0>(tup) == code) return tup;
}
#pragma warning(pop)
