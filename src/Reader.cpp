#include "Reader.hpp"
#include "Init.hpp"
#include "Output.hpp"
#include "State.hpp"
#include "Databases.hpp"
#include "SerialPool.hpp"
#include "Lights.hpp"

// https://github.com/SAP/odbc-cpp-wrapper
#include <odbc/Connection.h>
#include <odbc/PreparedStatement.h>
#include <odbc/ResultSet.h>
#include <odbc/Exception.h>


inline
void pause_exit()
{
	if (get_log_lvl() > 0)
		system("pause");
	exit(0);
}


bool is_barcode_valid(std::string_view barcode)
{
	return true;
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
				dprintf("\n%s: read %s\n", serial_port.getPort().c_str(), barcode.c_str());
				if (!is_barcode_valid(barcode))
				{
					dprintf(msg<2>());
					light(LightsEnum::Deny);
					continue;
				}

				state.id = barcode;

				odbc::ResultSetRef rs = select_from_cars();

				if (rs->next())
				{
					// save info about authorization
					store_info(serial_port.getPort().c_str(), barcode.c_str());

					state.min_weight = (double)*rs->getInt(1);

					double corr = (double)*rs->getInt(2);
					if (0 < corr && corr < 100)
					{
						corr = 1.0 + corr / 100;
						state.corr = [corr_k=corr](double inp) -> double
									 { return inp * corr_k; };
					}
					else
					{
						state.corr = [corr=corr](double inp) -> double
									 { return inp + corr; };
					}

					state.com = serial_port.getPort();
					dprintf("min_weight: %lf\ncom: %s\n\n",
						state.min_weight, state.com.c_str());
					set_authorized();

					light(LightsEnum::Acc);
				}
				else
				{
					std::string e = "cars_db returned nothing for id=" + state.id;
					dprintf(msg<7>());
					throw std::exception(e.c_str());

					// light deny
				}
			}
			catch (const std::exception& e)
			{
				dprintf("Exception while processing com input %s\n\n", e.what());
				dprintf(msg<6>());
				pause_exit();
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
