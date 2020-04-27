#define UNICODE
#include <windows.h>
#include <cstdio>
#include <string>
void die()
{
	PostQuitMessage(0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pwCmdLine, int nCmdShow)
{
	int argc;
	wchar_t **argv = reinterpret_cast<wchar_t**>(CommandLineToArgvW(GetCommandLineW(), &argc));

	if (argc != 3)
	{
		MessageBox(NULL, TEXT("ERROR"), TEXT("Invalid number of arguments"), 0);
		//return 0;
	}

	HANDLE h;
	wchar_t *_;
	unsigned int ms = wcstol(argv[2], &_, 10);
	CreateTimerQueueTimer(&h, NULL, (WAITORTIMERCALLBACK)die, nullptr, ms, 0, WT_EXECUTEDEFAULT);

	printf("displayinh.\n");
	MessageBoxW(NULL,
				argv[0],
				argv[1], MB_OK);
	return 0;
}
