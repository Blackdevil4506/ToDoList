#include "edge_handle.h"
static BOOL g_isHover = FALSE;


/*
    Window class name for the edge handle
*/
static const wchar_t* EDGE_HANDLE_CLASS = L"EdgeHandleWindow";

/*
    Window procedure for the edge handle
*/
static LRESULT CALLBACK EdgeHandle_WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (msg)
    {
        case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        // Background color
        COLORREF color = g_isHover
            ? RGB(110, 110, 110)   // hover
            : RGB(80, 80, 80);     // normal

        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen = CreatePen(PS_NULL, 0, color);

        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen   = SelectObject(hdc, pen);

        // Rounded rectangle (grip look)
        RoundRect(
            hdc,
            rc.left,
            rc.top,
            rc.right,
            rc.bottom,
            12,   // corner radius X
            12    // corner radius Y
        );

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
{
    if (!g_isHover)
    {
        g_isHover = TRUE;
        InvalidateRect(hwnd, NULL, TRUE);

        TRACKMOUSEEVENT tme = {0};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
    }
    return 0;
}

    case WM_MOUSELEAVE:
    {
        g_isHover = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_SETCURSOR:
    {
        SetCursor(LoadCursor(NULL, IDC_HAND));
        return TRUE;
    }

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}


/*
    Create the edge handle window
*/
HWND EdgeHandle_Create(HINSTANCE hInstance, EdgeSide side)
{
    // 1. Register window class
    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.lpfnWndProc   = EdgeHandle_WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = EDGE_HANDLE_CLASS;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    // 2. Get screen size
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 3. Edge handle size
    int handleWidth  = 16;
    int handleHeight = 100;   // small handle like your image

int x = (side == EDGE_LEFT)
    ? 0
    : screenWidth - handleWidth;

// center vertically
    int y = (screenHeight - handleHeight) / 2;


    // 5. Create window
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        EDGE_HANDLE_CLASS,
        L"",
        WS_POPUP,
        x, y,
        handleWidth,
        handleHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

/*
    Update handle position when side changes
*/
void EdgeHandle_UpdatePosition(HWND hwnd, EdgeSide side)
{
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int handleWidth = 16;

    int x = (side == EDGE_LEFT)
        ? 0
        : screenWidth - handleWidth;

    int handleHeight = 100;
    int y = (screenHeight - handleHeight) / 2;

    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        x,
        y,
        handleWidth,
        handleHeight,
        SWP_SHOWWINDOW
    );

}
