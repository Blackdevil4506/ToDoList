#define UNICODE
#define _UNICODE


#include <windows.h>
#include "app.h"

/*
    Entry point of a UNICODE Windows application.
*/
int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR     pCmdLine,
    int       nCmdShow
)
{
    /*
        Step 1: Create an AppState object.
        This will hold global app data like the main window handle.
    */
    AppState app;
    ZeroMemory(&app, sizeof(app));

    /*
        Step 2: Initialize the application.
        This creates the main sidebar window.
    */
    if (!App_Init(&app, hInstance)) {
        MessageBoxW(
            NULL,
            L"Application initialization failed",
            L"Error",
            MB_OK | MB_ICONERROR
        );
        return 0;
    }

    /*
        Step 3: Run the application message loop.
        This keeps the app alive until it exits.
    */
    return App_Run(&app);
}
