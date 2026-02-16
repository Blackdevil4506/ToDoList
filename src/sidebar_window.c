#include "sidebar_window.h"
#include "edge_handle.h"

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
    case WM_NCHITTEST:
{
    LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
    if (hit == HTCLIENT)
        return HTCLIENT;

    return HTTRANSPARENT;
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
    WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    SIDEBAR_CLASS,
    L"",
    WS_POPUP,
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

void Sidebar_Show(HWND hwnd, EdgeSide side)
{
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int panelWidth  = 400;
    int panelHeight = 500;

    int y = (screenHeight - panelHeight) / 2;

    int handleWidth = 16;   // same as edge handle width
    int startX, endX, step;

    if (side == EDGE_LEFT) {
        // panel starts hidden BEHIND the handle
        startX = handleWidth - panelWidth;
        endX   = handleWidth;
        step   = 16;
    } else {
        // panel starts hidden BEHIND the right handle
        startX = screenWidth - handleWidth;
        endX   = screenWidth - panelWidth - handleWidth;
        step   = -16;
    }

    // Place window off-screen and show it ONCE
    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        startX,
        y,
        panelWidth,
        panelHeight,
        SWP_SHOWWINDOW
    );

    // Animate
    for (int x = startX;
         (step > 0 ? x <= endX : x >= endX);
         x += step)
    {
        SetWindowPos(
            hwnd,
            HWND_TOPMOST,
            x,
            y,
            panelWidth,
            panelHeight,
            SWP_NOACTIVATE
        );

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);

        // 👇 THIS is the key
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(10);
    }

    // Snap to final position
    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        endX,
        y,
        panelWidth,
        panelHeight,
        SWP_NOACTIVATE
    );
}

void Sidebar_Hide(HWND hwnd)
{
    ShowWindow(hwnd, SW_HIDE);
}

BOOL Sidebar_IsVisible(HWND hwnd)
{
    return IsWindowVisible(hwnd);
}

