#include "app.h"
#include "sidebar_window.h"
#include "edge_handle.h"

BOOL App_Init(AppState* app, HINSTANCE hInstance)
{
    // Store application instance
    app->hInstance = hInstance;

    // Create main sidebar window and store its handle
    app->hwndMain = Sidebar_Create(app->hInstance);

    // If window creation failed, return FALSE
    if (!app->hwndMain) return FALSE;

    // Show the window on screen
    ShowWindow(app->hwndMain, SW_HIDE);

    // Force a paint/update immediately
    UpdateWindow(app->hwndMain);

    app->edgeSide = EDGE_LEFT;   // temporary, we’ll load from settings later
    app->hwndEdge = EdgeHandle_Create(hInstance, app->edgeSide, app);


    return TRUE;
}

int App_Run(AppState* app)
{
    MSG msg = {0};

    // Main Windows message loop
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
