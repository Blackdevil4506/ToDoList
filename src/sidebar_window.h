#pragma once
#include <windows.h>
#include "edge_handle.h"   // <-- ADD THIS

HWND Sidebar_Create(HINSTANCE hInstance);
void Sidebar_Show(HWND hwnd, EdgeSide side);
void Sidebar_Hide(HWND hwnd, EdgeSide side);
BOOL Sidebar_IsVisible(HWND hwnd);
