#include "Output.hpp"
#include "serialPool.hpp"
#include <chrono>
#include <thread>
#include <cstdio>

serial::Serial& SerialPool::bad_wait()
{
	if (get_log_lvl() > 0)
	{
		printf("Listening to COM ports\n");
	}

	using namespace std::chrono_literals;
	while (true)
	{
		for (auto &serial : ports)
		{
			if (serial.available()) return serial;
		}
		std::this_thread::sleep_for(1s);
	}
}