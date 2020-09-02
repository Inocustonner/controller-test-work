#pragma once
#include <serial/serial.h>		// https://github.com/wjwwood/serial
#include <vector>

struct SerialPool
{
public:
	// template<typename ...SArgs> // SArgs = serial::Serial...
	// SerialPool(SArgs&& ...serial_args) : ports{ std::move(serial_args)... } {}
	SerialPool() = default;
	SerialPool(std::vector<serial::Serial> &&_ports) : ports{ std::move(_ports) } {}
	
	[[nodiscard]]
	serial::Serial& bad_wait();	// returns first port that recieved data

private:
	std::vector<serial::Serial> ports;
};
