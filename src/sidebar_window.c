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

        // Create a dark background brush
        HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 30));

        // Fill the window's paint area with the brush
        FillRect(hdc, &ps.rcPaint, bgBrush);
        // Clean up GDI object
        DeleteObject(bgBrush);

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

