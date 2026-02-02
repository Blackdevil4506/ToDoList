#ifndef EDGE_HANDLE_H
#define EDGE_HANDLE_H

#include <windows.h>

/*
    Which side of the screen the edge handle is on
*/
typedef enum {
    EDGE_LEFT,
    EDGE_RIGHT
} EdgeSide;

/*
    Creates the edge handle window.
    - hInstance: application instance
    - side: EDGE_LEFT or EDGE_RIGHT
*/
HWND EdgeHandle_Create(HINSTANCE hInstance, EdgeSide side);

/*
    Updates the edge handle position when side changes.
*/
void EdgeHandle_UpdatePosition(HWND hwnd, EdgeSide side);

#endif
