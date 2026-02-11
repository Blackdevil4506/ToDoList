#pragma once
#include <windows.h>

HWND Sidebar_Create(HINSTANCE hInstance);
void Sidebar_Show(HWND hwnd);
void Sidebar_Hide(HWND hwnd);
BOOL Sidebar_IsVisible(HWND hwnd);

