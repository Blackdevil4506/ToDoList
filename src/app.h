#ifndef APP_H
#define APP_H

#include <windows.h>
#include "edge_handle.h"   // IMPORTANT

typedef struct {
    HINSTANCE hInstance;

    // Edge handle
    HWND hwndEdge;
    EdgeSide edgeSide;

    // Main panel (sidebar)
    HWND hwndMain;

} AppState;

/* App lifecycle */
BOOL App_Init(AppState* app, HINSTANCE hInstance);
int  App_Run(AppState* app);

#endif
