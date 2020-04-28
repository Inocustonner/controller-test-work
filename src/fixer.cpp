#include "shared.hpp"
#include "init.hpp"
#include "fixer.h"
#include "lights.hpp"

#include <string_view>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <cstdarg>

odbc::EnvironmentRef odbc_env;
odbc::ConnectionRef cars_db, store_db, store_info_db;
odbc::ConnectionRef debug_db;	// externed

// odbc::ConnectionRef debug_db;
// in cars_db cars table is used
// in store_db info table is used
std::thread reader_thread;		// thread that reads from serial port

std::atomic<bool> authorized = false;	// has this driver pass authorization
extern bool debug = false;
extern int log_level = 0;
std::string suffix = "";
State state{};


inline
void reset_state()
{
	state = State{};
	state.min_weight = state.reset_thr;
	authorized = false;
}


void store(double p1)
{
	try
	{
		// id, code id, mass, timestamp(default like)
		odbc::PreparedStatementRef ps = store_db->prepareStatement("INSERT INTO info (com, event_id, id, weight, inp_weight) VALUES(?, ?, ?, ?, ?)");
		ps->setString(1, state.com);
		ps->setInt(2, state.event_id);
		ps->setString(3, state.id);
		ps->setInt(4, (int)(p1 - state.corr));
		ps->setInt(5, (int)p1);
		ps->executeUpdate();
	}
	catch (const std::exception &e)
	{
		dprintf("Error while storing to store_db:\n\t%s\n", e.what());
		dprintf(msg<5>());
	}
}


inline
double phase1(double p1, bool is_stable)
{
	if (std::abs(p1 - state.p0) > state.store_diff && is_stable)
	{
		store(p1);
	}
	return p1 - state.corr;
}


inline
void phase0(double p1)
{
	if (state.p0 > p1 && state.reset_thr > state.p0) // reset on entering
	{
		printf("p0: %lf state.reset_thr: %lf\n", state.p0, state.reset_thr);
		dprintf("resetting\n");
		reset_state();
	}
}


double fix(double p1, bool is_stable)
{
	double ret_value = p1;
	if (authorized)
	{
		if (p1 < state.min_weight)
		{
			state.phase = 0;
			phase0(p1);
		}
		else
		{
			if (get_last_light_enum() == LightsEnum::ACCEPT) light(LightsEnum::WAIT);

			state.phase = 1;
			ret_value = phase1(p1, is_stable);
		}
	}
	else if (p1 >= state.reset_thr)
	{
		dprintf(msg<1>());
	}	

	state.p0 = p1;
	return ret_value;		
}


inline
void store_info(const std::string &com, const std::string &barcode)
{
	//															 event_id, com, barcode(event_id yet), (default timestamp)
	odbc::PreparedStatementRef ps = store_info_db->prepareStatement("INSERT INTO info VALUES(?, ?, ?)"\
																 "ON CONFLICT (event_id) DO UPDATE SET "
																 "event_id=EXCLUDED.event_id, com=EXCLUDED.com, barcode=EXCLUDED.barcode, ts=CURRENT_TIMESTAMP");
	ps->setInt(1, state.event_id);
	ps->setString(2, com);
	ps->setString(3, barcode);
	ps->executeUpdate();
}


inline
void inc_event_id()
{
	auto ps = store_db->prepareStatement("SELECT nextval('event_id')");
	auto rs = ps->executeQuery();
	if (rs->next())
	{
		state.event_id = (int)*rs->getLong(1);
	}
}


bool is_barcode_valid(std::string_view barcode)
{
	return true;
}


void serial_read(std::vector<PortInfo> pi)
{
	std::vector<serial::Serial> ports(std::size(pi)); // used once
	try
	{

		for (size_t i = 0; i < std::size(pi); ++i)
		{
			ports[i].setPort(pi[i].name);
			ports[i].setBaudrate(pi[i].baudrate);
			ports[i].setBytesize(pi[i].bytesize);
			ports[i].setParity(pi[i].parity);
			ports[i].open();
		}

		SerialPool serial_pool{ std::move(ports) };
		while (true)
		{
			serial::Serial &serial_port = serial_pool.bad_wait();
			const size_t max_line_sz = 65536;
			if (authorized)
			{
				// dprintf("Error double authorization\n");
				dprintf(msg<0>());
				serial_port.readline(max_line_sz, suffix); // deny given data
				light(LightsEnum::DENY);
				continue;
			}

			try
			{
				std::string barcode = serial_port.readline(max_line_sz, suffix); // from bar code
				barcode.pop_back(); // remove eol symbol
				dprintf("%s: read %s\n", serial_port.getPort().c_str(), barcode.c_str());
				
				if (!is_barcode_valid(barcode))
				{
					dprintf(msg<2>());
					light(LightsEnum::DENY);
					continue;
				}
				state.id = barcode;

				odbc::PreparedStatementRef ps = cars_db->prepareStatement("SELECT weight, corr FROM cars_table WHERE id=?");
				ps->setString(1, state.id);
				odbc::ResultSetRef rs = ps->executeQuery();

				if (rs->next())
				{
					inc_event_id();

					// save info about authorization
					store_info(serial_port.getPort(), barcode);

					state.min_weight = (double)*rs->getInt(1);
					state.corr = (double)*rs->getInt(2);
					state.com = serial_port.getPort();
					dprintf("min_weight: %lf\ncorr: %lf\ncom: %s\n",
							state.min_weight, state.corr, state.com.c_str());
					authorized = true;

					light(LightsEnum::ACCEPT);
				}
				else
				{
					std::string e = "cars_db returned nothing for id=" + state.id;
					dprintf(msg<7>());
					throw std::exception(e.c_str());

					light(LightsEnum::DENY);
				}
			}
			catch (const std::exception &e)
			{
				dprintf("Exception while processing COM:\n\t%s\n", e.what());
				dprintf(msg<6>());
			}
		}
	}
	catch (const std::exception &e)
	{
		dprintf(msg<3>());
		dprintf("Exception in " __FUNCTION__ ":\n\t%s\n", e.what());
		if (log_level > 0)
			system("pause");
		exit(0);
	}
}


bool init_fixer(const char *ini_filename)
{
	setlocale(LC_ALL, "Russian");
	Settings set = read_settings(ini_filename, state);
	init_interface();			// for async msg boxes
	if (!init_db(set, odbc_env, cars_db, store_db, store_info_db, debug_db))
	{
		return false;
	}

	reset_state();
	light(LightsEnum::WAIT);

	reader_thread = std::thread(serial_read, set.pi);
	reader_thread.detach();
	return true;
}
