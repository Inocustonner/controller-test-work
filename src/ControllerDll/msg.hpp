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
	MSG(0, MsgType::Error,			"Error: double authorization\n",						"Авто на весах\nПопытка авторизации отклонена"),
	NOREP_MSG(1, MsgType::Error,	"Error: Unauthorized driver\n",							"Неавторизованный водитель"),
	MSG(2, MsgType::Error,			"Error: Invalid barcode\n",								"Неверный формат штрих кода"),
	NOREP_MSG(3, MsgType::Error,	"Com port connecting error\n",							"Ошибка подключения com port'a"),
	NOREP_MSG(4, MsgType::Warning,	"Unknown option ignored\n",								"Ошибка ini файла\nНеизвестная опция проигнорирована"),
	MSG(5, MsgType::Error,			"Error while writing to a db\n",						"Ошибка записи базы данных"),
	MSG(6, MsgType::Error,			"Error while reading from a db\n",						"Ошибка чтения базы данных"),
	MSG(7, MsgType::Error,			"Error car id not found\n",								"Авто в базе данных не найдено"),
	MSG(8, MsgType::Error,			"Error driver not found\n",								"Водитель в базе данных не найден"),
	MSG(9, MsgType::Error,			"Error decoding ini file\n",							"Ошибка ini файла\nНе удалось расшифровать"),
	MSG(10, MsgType::Error,			"Error connecting to db\n", 							"Ошибка базы данных\nНе удалось подключиться"),
	MSG(11, MsgType::Error,			"Error storing to store_info\n",						"Ошибка записи базы данных\nНе удалось записать в store_info"),
	MSG(12, MsgType::Error,			"Error openning ini file\n",							"Ошибка ini файла\nНе удалось открыть файл"),
	MSG(13, MsgType::Warning,		"Warning no COM ports connected\n",						"Не указан порт подключения сканнера"),
	MSG(14, MsgType::Warning,		"Warning reauthorization, new values will be used\n",	"Произведена повторная авторизация")
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
