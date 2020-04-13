#include "serialPool.hpp"
#include <chrono>
#include <thread>

serial::Serial& SerialPool::bad_wait()
{
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
