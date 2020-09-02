#include "Retranslator.hpp"

int main() {
	auto r = Retranslator("COM2", "COM1");
	r.start();
	return 0;
}