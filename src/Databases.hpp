#include <odbc/Forwards.h>

void store(const char* com, const char* id,
	int corr_weight, int inp_weight) noexcept;
odbc::ResultSetRef select_from_cars();
void store_info(const char* com, const char* barcode);

// void set_cars_db(odbc::ConnectionRef new_cars_db);
// void set_store_db(odbc::ConnectionRef new_store_db);
// void set_store_info_db(odbc::ConnectionRef new_store_info_db);

odbc::ConnectionRef& get_store_db();
odbc::ConnectionRef& get_store_info_db();
odbc::ConnectionRef& get_cars_db();

odbc::EnvironmentRef get_odbc_env();
