#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cstdio>
#include <string>
#include <tchar.h>

#define CLOSE_TIMER_IDI 0xD13

HINSTANCE hInst;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

LPWSTR title;
LPWSTR body;

int text_center_offset = 30;// 10px
int window_w, window_h;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pwCmdLine, int nCmdShow)
{
	int argc;
	wchar_t **argv = reinterpret_cast<wchar_t**>(CommandLineToArgvW(GetCommandLineW(), &argc));

	if (argc < 3 || argc > 4)
	{
		MessageBox(NULL, TEXT("ERROR"), TEXT("Invalid number of arguments"), 0);
		return 0;
	}

    wchar_t* _;
    unsigned int ms;
    if (argc == 3)
    {
        body = argv[0];
        title = argv[1];
        ms = wcstol(argv[2], &_, 10);
    }
    else
    {
        body = argv[1];
        title = argv[2];
        ms = wcstol(argv[3], &_, 10);
    }

	//printf("displayinh.\n");
	//MessageBoxW(NULL,
	//			argv[0],
	//			argv[1], MB_OK);

    WNDCLASSEXW wcex;


    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = argv[0];
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    //get text size
    HDC hDC = GetDC(NULL);
    RECT r1 = { 0, 0, 0, 0 };
    RECT r2 = {};

    HFONT hFont, hOldFont;

    // Retrieve a handle to the variable stock font.  

    int text_sz;

    DrawTextW(hDC, body, _tcslen(body), &r1, DT_CALCRECT);
    text_sz = r1.right;

    r1.right += text_center_offset * 2;
    r1.bottom += 80; // adjustment

    DrawTextW(hDC, title, _tcslen(title), &r2, DT_CALCRECT);
    r2.right += text_center_offset * 2;
    ReleaseDC(NULL, hDC);

    r1.right = max(max(r1.right, r2.right), 300);

    AdjustWindowRect(&r1, wcex.style, FALSE);

    window_w = r1.right - r1.left;
    window_h = r1.bottom - r1.top;

    text_center_offset = 5;
    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindow explained:
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindowW(
        wcex.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        (screen_w - window_w) / 2, (screen_h - window_h) / 2,
        window_w, window_h,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, CLOSE_TIMER_IDI, ms, nullptr);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:
    {
        int width = 90, height = 30;
        int x = (window_w - width) / 2, y = window_h - height - 40;

        HWND hwndButton = CreateWindowW(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"OK",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            x,         // x position 
            y,         // y position 
            width,        // Button width
            height,        // Button height
            hWnd,     // Parent window
            NULL,       // No menu.
            hInst,
            NULL);      // Pointer not needed.
    }break;

    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);

        HFONT hFont = (HFONT)GetStockObject(SYSTEM_FONT);
        SelectObject(hdc, hFont);
        // Here your application is laid out.
        // For this introduction, we just print out "Hello, Windows desktop!"
        // in the top left corner.
        TextOutW(hdc,
            text_center_offset, 5,
            body, _tcslen(body));
        // End application-specific layout section.

        EndPaint(hWnd, &ps);
    }break;
    case WM_TIMER:
    {
        switch (wParam)
        {
            case CLOSE_TIMER_IDI:
                KillTimer(hWnd, CLOSE_TIMER_IDI);
                PostQuitMessage(0);
                break;
            default:
                break;
        }
    }
    case WM_COMMAND:
        PostQuitMessage(0); // button clicked
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}