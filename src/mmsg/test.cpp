#include <Windows.h>
#include <mmsg/mmsg.hpp>
#include <thread>
#include <chrono>
#include <serial/serial.h>
#include <numeric>
#undef min

int main()
{
	init_interface();
	do
	{
		mMsgBox(L"\"Ã€ –”—— »≈\"", L"\"— Õ¿Ã» ¡Œ√\"", 10000);
		//mMsgBox(L"1234", L"gge", 60);
		//mMsgBox(L"1235", L"gge", 80);
		std::this_thread::sleep_for(std::chrono::milliseconds(20000));
	} while (1);
	
	printf("Fine\n");
	return 0;
}
