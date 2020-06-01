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
	return barvec.size() == 4;
}


std::vector<std::string> splitBy(std::string& s, char delim)
{
    size_t pos = 0, ppos = 0;
    std::vector<std::string> vec;
    while ((pos = s.find(delim, ppos)) != std::string::npos)
	{
        std::string token = s.substr(ppos, pos - ppos);
        vec.push_back(token);
        ppos = pos + 1;
    }
    vec.push_back(s.substr(ppos, std::string::npos));
    return vec;
}


void com_reader(std::vector<Port_Info> pi_v, const std::string suffix)
{
	std::vector<serial::Serial> ports(std::size(pi_v));
	try
	{
		for (size_t i = 0; i < std::size(pi_v); ++i)
		{
			ports[i].setPort(pi_v[i].name);
			ports[i].setBaudrate(pi_v[i].baudrate);
			ports[i].setBytesize(static_cast<serial::bytesize_t>(pi_v[i].byte_size));
			ports[i].setParity(static_cast<serial::parity_t>(pi_v[i].parity));
			ports[i].open();
		}
		SerialPool serial_pool{ std::move(ports) };
		light(LightsEnum::Wait);
		while (true)
		{
			try
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
				const std::vector barvec = splitBy(barcode, '#');

				dprintf("\n%s: read %s\n", serial_port.getPort().c_str(), barcode.c_str());
				if (!is_barcode_valid(barvec))
				{
					dprintf(msg<2>());
					light(LightsEnum::Deny);
					continue;
				}
				state.id = barvec[1];
				const std::string driver_id = barvec[2];

				data_s* data_p = select_from_cars();
				if (data_p)
				{
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
					std::string gn = reinterpret_cast<const char*>(data_p->body());

					state.com = serial_port.getPort();
					store_info(serial_port.getPort().c_str(), barcode.c_str(), gn.c_str(), driver_id.c_str());
					set_authorized();

					light(LightsEnum::Acc);
				}
				else
				{
					std::string e = "cars_db returned nothing for id=" + state.id;
					dprintf(msg<7>());
					throw std::exception(e.c_str());

					// light deny
					light(LightsEnum::Deny);
				}
			}
			catch (const std::exception& e)
			{
				dprintf("Exception while processing com input %s\n\n", e.what());
				dprintf(msg<6>());
				//pause_exit();
			}
		}
	}
	catch (const std::exception& e)
	{
		dprintf("Exception in com_reader init %s\n", e.what());
		dprintf(msg<3>());
		pause_exit();
	}
}