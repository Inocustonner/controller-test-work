#define UNICODE
#define _UNICODE
#include <windows.h>
#undef min

#include <cstdio>
#include <string>
#include <tchar.h>
#include <algorithm>

#define CLOSE_TIMER_IDI 0xD13

HINSTANCE hInst;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

std::wstring title;
std::wstring body;

void str_replace(std::wstring& str, const std::wstring& pattern, const std::wstring& replacement)
{
    size_t pos = str.find(pattern, 0);
    while (pos != std::wstring::npos)
    {
        str.replace(std::begin(str) + pos,
            std::min(std::begin(str) + pos + pattern.length(), std::end(str)),
            replacement);
        pos = str.find(pattern, pos + pattern.length());
    }
}

DWORD draw_text_flags = DT_CENTER | DT_EXTERNALLEADING | DT_WORDBREAK;
int window_w, window_h;
int button_w = 90, button_h = 30;

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
    // replace \\n with \n
    str_replace(body, L"\\n", L"\n");

    WNDCLASSEXW wcex;


    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;

    if (title == L"Error")
    {
        wcex.hIcon = LoadIcon(NULL, IDI_ERROR);
        wcex.hIconSm = LoadIcon(NULL, IDI_ERROR);
    }
    else if (title == L"Warning")
    {
        wcex.hIcon = LoadIcon(NULL, IDI_WARNING);
        wcex.hIconSm = LoadIcon(NULL, IDI_WARNING);
    }
    else
    {
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }

    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = argv[0];
    // wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_ERROR)
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

    // Retrieve a handle to the variable stock font.  


    DrawTextW(hDC, body.c_str(), body.length(), &r1, DT_CALCRECT | DT_CENTER);

    DrawTextW(hDC, title.c_str(), title.length(), &r2, DT_CALCRECT);
    ReleaseDC(NULL, hDC);

    r1.right = max(max(r1.right, r2.right), 300);

    AdjustWindowRect(&r1, wcex.style, TRUE);

    window_w = r1.right - r1.left;
    window_h = r1.bottom - r1.top + button_h + 30;

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
        title.c_str(),
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
        RECT rc = {};
        GetClientRect(hWnd, &rc);

        int x = ((rc.right - rc.left) - button_w) / 2, y = (rc.bottom - rc.top) - button_h - 5;

        HWND hwndButton = CreateWindowW(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"OK",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            x,         // x position 
            y,         // y position 
            button_w,        // Button width
            button_h,        // Button height
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
        RECT rc = {};
        GetClientRect(hWnd, &rc);

        DrawTextW(hdc, body.c_str(), body.length(), &rc, DT_CENTER | DT_EXTERNALLEADING | DT_WORDBREAK);

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