#include "Databases.hpp"
#include "Output.hpp"
#include "State.hpp"

#include <Control.hpp>
#include <Error.hpp>
#include <myassert.hpp>

static void write_str_to_data(data_s* data_p, const char* cstr)
{
	data_p->type = DataType::Str;
	data_p->size = std::strlen(cstr) + 1;
	std::memcpy(data_p->body(), cstr, data_p->size);
}

inline
void write_int_to_data(data_s* data_p, int i)
{
	data_p->type = DataType::Int;
	data_p->size = sizeof(i);
	*reinterpret_cast<int*>(data_p->body()) = i;
}


void store(const char* com, const char* id,
	int corr_weight, int inp_weight)
{
	std::lock_guard<std::mutex> guard(Control::get_main_mutex());

	//Control::lockMutexStore();
	command_s* cmd_p = Control::get_command();
	cmd_p->cmd = Cmd::Store_Store;

	data_s* data_p = Control::next_data();
	write_str_to_data(data_p, com);

	data_p = Control::next_data(data_p);
	write_int_to_data(data_p, state.event_id);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, id);

	data_p = Control::next_data(data_p);
	write_int_to_data(data_p, corr_weight);

	data_p = Control::next_data(data_p);
	write_int_to_data(data_p, inp_weight);
	//Control::releaseMutexStore();
	Control::SetEventMain();
	Control::syncDb();
}


void store_info(const char* com, const char* barcode, const char* gn, const char* driver_id)
{
	std::lock_guard<std::mutex> guard(Control::get_main_mutex());

	command_s* cmd_p = Control::get_command();
	cmd_p->cmd = Cmd::Store_Store_Info;

	data_s* data_p = Control::next_data();
	write_str_to_data(data_p, driver_id);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, com);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, barcode);

	data_p = Control::next_data(data_p);
	write_str_to_data(data_p, gn);

	// no need to wait
	Control::SetEventMain();
	Control::syncDb();
	if (cmd_p->cmd == Cmd::Done)
	{
		data_p = Control::next_data();
		massert(data_p->type == DataType::Int);
		state.event_id = *reinterpret_cast<int*>(data_p->body());
	}
	else
	{
		throw ctrl::error("Store Info error: %s\n", reinterpret_cast<const char*>(data_p->body()));
	}
}


data_s* select_from_cars()
{
	std::lock_guard<std::mutex> guard(Control::get_main_mutex());

	command_s* cmd_p = Control::get_command();
	data_s* data_p = Control::next_data(nullptr);

	cmd_p->cmd = Cmd::Read_Cars;
	data_p->type = DataType::Str;
	data_p->size = std::size(state.id) + 1;
	std::memcpy(data_p->body(), state.id.c_str(), data_p->size);

	Control::SetEventMain();
	Control::syncDb();
	if (cmd_p->cmd == Cmd::Done)
	{
		throw ctrl::error("Select from cars error: %s\n", reinterpret_cast<const char*>(data_p->body()));
		return Control::next_data(nullptr);
	}
	else
	{
		throw ctrl::error("Select from cars error: %s\n", reinterpret_cast<const char*>(data_p->body()));
	}
}