#pragma once
#include <windows.h>

typedef struct AppState {
    HINSTANCE hInstance;
    HWND hwndMain;
} AppState;

BOOL App_Init(AppState* app, HINSTANCE hInstance);
int  App_Run(AppState* app);
