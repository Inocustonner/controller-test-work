#pragma once
#include <exception>
#include <string>
#include <algorithm>

namespace ctrl
{
	constexpr size_t buffer_sz = 1024;
	class error : public std::exception
	{
		char msg[buffer_sz];
		int ret_code = 0;

	public:
		template<typename... Args>
		error(const char* fmt, Args&& ...args) noexcept 
			: std::exception{}
		{
			snprintf(msg, buffer_sz, fmt, args...);
		}

		template<typename... Args>
		error(const char* fmt, Args&& ...args, int ret_code) noexcept
			: std::exception{}
		{
			this->ret_code = ret_code;
			snprintf(msg, buffer_sz, fmt, args...);
		}

		error(std::string&& str) noexcept
		{
			std::memcpy(msg, str.c_str(), std::min(std::size(str), std::size(msg)));
		}

		error(std::string&& str, int ret_code) noexcept
		{
			this->ret_code = ret_code;
			std::memcpy(msg, str.c_str(), std::min(std::size(str), std::size(msg)));
		}

		int code() const noexcept
		{
			return this->ret_code;
		}

		const char* what() const noexcept override
		{
			return reinterpret_cast<const char*>(msg);
		}
	};
}