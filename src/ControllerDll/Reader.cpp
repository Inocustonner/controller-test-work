#include "Reader.hpp"
#include "Init.hpp"
#include "Output.hpp"
#include "State.hpp"
#include "Databases.hpp"
#include "SerialPool.hpp"
#include "Lights.hpp"
#include "Control.hpp"

// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Connection.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>
#include <odbc/Exception.h>

#include <Error.hpp>

#include <myassert.hpp>

inline
void pause_exit()
{
	if (get_log_lvl() > 0)
		system("pause");
	exit(0);
}


bool is_barcode_valid(std::vector<std::string> barvec)
{
	return barvec.size() == 3;
}


std::vector<std::string> parse_barcode(std::string s, char delim)
{
	std::vector<std::string> vec;
	vec.reserve(3);
	constexpr int bar_type_size = 2;
	constexpr int car_id_size = 8;
	constexpr int driver_id_size = 8;

	//	...01#12345678#87654321#\r
	size_t pos = 0;
	pos = s.substr(0, std::size(s) - 1).rfind(delim);
	vec.insert(std::begin(vec), { std::begin(s) + pos + 1, std::begin(s) + pos + 1 + driver_id_size });

	pos = s.substr(0, pos).rfind(delim);
	vec.insert(std::begin(vec), { std::begin(s) + pos + 1, std::begin(s) + pos + 1 + car_id_size });

	vec.insert(std::begin(vec), s.substr(pos - 2, bar_type_size));
	return vec;
}


void com_reader(std::vector<Port_Info> pi_v, const std::string suffix, bool udentified_car_allowed)
{
	SerialPool serial_pool;
	try
	{
		std::vector<serial::Serial> ports(std::size(pi_v));
		for (size_t i = 0; i < std::size(pi_v); ++i)
		{
			ports[i].setPort(pi_v[i].name);
			ports[i].setBaudrate(pi_v[i].baudrate);
			ports[i].setBytesize(static_cast<serial::bytesize_t>(pi_v[i].byte_size));
			ports[i].setParity(static_cast<serial::parity_t>(pi_v[i].parity));
			ports[i].open();
		}
		serial_pool = SerialPool{ std::move(ports) };
	}
	catch (const std::exception& e)
	{
		dprintf("Exception in com_reader init %s\n", e.what());
		dprintf(msg<3>());
		pause_exit();
	}

	light(LightsEnum::Wait);
	while (true)
	{
		serial::Serial& serial_port = serial_pool.bad_wait();
		constexpr size_t max_line_sz = (1 << 16);
		if (authorized())
		{
			dprintf(msg<0>());
			serial_port.readline(max_line_sz, suffix);
			light(LightsEnum::Deny);
			continue;
		}
		std::string barcode = serial_port.readline(max_line_sz, suffix);

		barcode.pop_back();	// remove suffix
		const std::vector barvec = parse_barcode(barcode, '#');

		dprintf("\n%s: read %s\n", serial_port.getPort().c_str(), barcode.c_str());
		if (!is_barcode_valid(barvec))
		{
			dprintf(msg<2>());
			light(LightsEnum::Deny);
			continue;
		}
		state.id = barvec[1];
		const std::string driver_id = barvec[2];


		data_s* data_p;
		try
		{
			data_p = select_from_cars();
		}
		catch (const ctrl::error& e)
		{
			if (!udentified_car_allowed)
			{
				dprintf(e.what());
				dprintf(msg<7>());
			}
			data_p = nullptr;
		}

		try
		{
			std::string gn;
			LightsEnum le = LightsEnum::Deny;
			if (data_p)
			{
				le = LightsEnum::Acc;
				massert(data_p->type == DataType::Int);
				state.min_weight = static_cast<double>(*reinterpret_cast<int*>(data_p->body()));

				data_p = Control::next_data(data_p);
				massert(data_p->type == DataType::Int);
				double tmp_corr = static_cast<double>(*reinterpret_cast<int*>(data_p->body()));

				if (0 < tmp_corr && tmp_corr < 100)
				{
					tmp_corr = 1.0 + tmp_corr / 100;
					state.corr = [mw = state.min_weight, corr_k = tmp_corr](double inp) -> double
					{ return (inp - mw) * corr_k; };
				}
				else
				{
					state.corr = [corr = tmp_corr](double inp) -> double
					{ return inp + corr; };
				}

				data_p = Control::next_data(data_p);
				massert(data_p->type == DataType::Str);
				gn = reinterpret_cast<const char*>(data_p->body());
			}
			else if (udentified_car_allowed)
			{
				state.id = "";
				gn = "";
				state.corr = [](double inp) -> double { return inp; };
				state.min_weight = default_min_weight;
			}
			else// invalid car
			{
				light(LightsEnum::Deny);
				continue;
			}

			state.com = serial_port.getPort();

			store_info(state.com.c_str(), barcode.c_str(), gn.c_str(), driver_id.c_str(), udentified_car_allowed);
			set_authorized();

			light(le);
		}
		catch (const ctrl::error& e)
		{
			light(LightsEnum::Deny);
			dprintf(e.what());
			dprintf(msg<11>());
		}
	}
}
