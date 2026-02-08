#include "edge_handle.h"

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

            // Visible edge handle color (temporary)
            HBRUSH brush = CreateSolidBrush(RGB(80, 80, 80));
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);

            EndPaint(hwnd, &ps);
            return 0;
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
    int handleWidth  = 30;
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

    int handleWidth = 30;

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
