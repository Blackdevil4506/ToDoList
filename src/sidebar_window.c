#include "sidebar_window.h"

static const wchar_t* SIDEBAR_CLASS = L"SidebarWindowClass";

/*
    Window Procedure:
    This function receives all messages/events for the sidebar window
*/
static LRESULT CALLBACK Sidebar_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // 1. Get the full window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    int windowWidth  = clientRect.right;
    int windowHeight = clientRect.bottom;

    // 2. Paint sidebar background
    HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    // 3. Define thin strip size
    int stripWidth = 10;

    // 4. Calculate strip position (RIGHT side)
    RECT stripRect;
    stripRect.left   = windowWidth - stripWidth;
    stripRect.top    = 0;
    stripRect.right  = windowWidth;
    stripRect.bottom = windowHeight;

    // 5. Paint the thin strip
    HBRUSH stripBrush = CreateSolidBrush(RGB(60, 60, 60));
    FillRect(hdc, &stripRect, stripBrush);
    DeleteObject(stripBrush);

    EndPaint(hwnd, &ps);
    return 0;
}


    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/*
    Creates the main sidebar window and returns its HWND
*/
HWND Sidebar_Create(HINSTANCE hInstance)
{
    WNDCLASSW wc = {0};

    wc.lpfnWndProc   = Sidebar_WndProc;   // Message handler
    wc.hInstance     = hInstance;         // App instance
    wc.lpszClassName = SIDEBAR_CLASS;     // Unique class name
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,                      // Extended window style
        SIDEBAR_CLASS,          // Window class name
        L"SidebarApp",          // Window title
        WS_OVERLAPPEDWINDOW,    // Normal window for now
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        500,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    return hwnd;
}
void Sidebar_Show(HWND hwnd)
{
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
}

void Sidebar_Hide(HWND hwnd)
{
    ShowWindow(hwnd, SW_HIDE);
}

BOOL Sidebar_IsVisible(HWND hwnd)
{
    return IsWindowVisible(hwnd);
}

