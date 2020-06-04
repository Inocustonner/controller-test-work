#include <odbc/Forwards.h>

struct data_s;

void store(const char* com, const char* id,
	int corr_weight, int inp_weight);

data_s* select_from_cars();

void store_info(const char* com, const char* barcode, const char* gn, const char* driver_id);