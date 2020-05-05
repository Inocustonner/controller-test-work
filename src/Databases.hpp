#include <odbc/Forwards.h>

void store(const char *com, const char *id,
	int event_id, int corr_weight, int inp_weight) noexcept;

void set_store_db(odbc::ConnectionRef&& new_store_db);
void set_store_info_db(odbc::ConnectionRef&& new_store_info_db);
void set_cars_db(odbc::ConnectionRef&& new_cars_db);
odbc::EnvironmentRef get_odbc_env();
