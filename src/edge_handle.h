#ifndef EDGE_HANDLE_H
#define EDGE_HANDLE_H

#include <windows.h>

/* Forward declaration (no include!) */
typedef struct AppState AppState;

/* Edge side enum */
typedef enum {
    EDGE_LEFT,
    EDGE_RIGHT
} EdgeSide;

/* API */
HWND EdgeHandle_Create(
    HINSTANCE hInstance,
    EdgeSide side,
    AppState* app
);

void EdgeHandle_UpdatePosition(HWND hwnd, EdgeSide side);

#endif
