#ifndef APP_H
#define APP_H

#include <windows.h>
#include "edge_handle.h"

typedef struct AppState {
    HINSTANCE hInstance;
    HWND hwndEdge;
    EdgeSide edgeSide;
    HWND hwndMain;

    BOOL sidebarVisible;
} AppState;

/* App lifecycle */
BOOL App_Init(AppState* app, HINSTANCE hInstance);
int  App_Run(AppState* app);

#endif
