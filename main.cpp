#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include "resource.h"

#define ID_ADD 1
#define ID_REMOVE 2
#define ID_LISTBOX 3

HWND hListBox;
std::vector<std::wstring> tasks;

void SaveTasks() {
    FILE* file = _wfopen(L"tasks.txt", L"w, ccs=UNICODE");
    if (!file) return;

    for (const auto& task : tasks) {
        fwprintf(file, L"%s\n", task.c_str());
    }

    fclose(file);
}

void LoadTasks() {
    tasks.clear();

    FILE* file = _wfopen(L"tasks.txt", L"r, ccs=UNICODE");
    if (!file) return;

    wchar_t buffer[512];
    while (fgetws(buffer, 512, file)) {
        // Remove trailing newline
        buffer[wcslen(buffer) - 1] = 0;
        tasks.emplace_back(buffer);
    }

    fclose(file);
}

// Refresh ListBox UI
void RefreshListBox() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& task : tasks)
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)task.c_str());
}

// Dialog procedure for getting task input
INT_PTR CALLBACK AddTaskDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static wchar_t* inputBuffer;
    switch (uMsg) {
        case WM_INITDIALOG:
            inputBuffer = (wchar_t*)lParam;
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                GetDlgItemTextW(hwndDlg, 1001, inputBuffer, 256);
                EndDialog(hwndDlg, IDOK);
                return TRUE;
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

// Create a very simple custom input dialog using MessageBox style fallback
bool ShowInputDialog(HWND parent, wchar_t* buffer, int length) {
    HWND hwndInput = CreateWindowExW(
        0, L"EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 10, 180, 25,
        parent, (HMENU)1001,
        (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE),
        nullptr
    );

    if (DialogBoxParamW(nullptr, MAKEINTRESOURCE(101), parent, AddTaskDialogProc, (LPARAM)buffer) == IDOK) {
        return true;
    }

    return false;
}

// Main window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            LoadTasks();

            hListBox = CreateWindowW(L"LISTBOX", nullptr,
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
                10, 10, 180, 280,
                hwnd, (HMENU)ID_LISTBOX,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                nullptr);

            CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE,
                10, 300, 80, 30,
                hwnd, (HMENU)ID_ADD,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                nullptr);

            CreateWindowW(L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE,
                110, 300, 80, 30,
                hwnd, (HMENU)ID_REMOVE,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                nullptr);

            RefreshListBox();
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_ADD) {
                wchar_t buffer[256] = {};
                if (ShowInputDialog(hwnd, buffer, 256)) {
                    tasks.emplace_back(buffer);
                    RefreshListBox();
                    SaveTasks();
                }
            } else if (LOWORD(wParam) == ID_REMOVE) {
                int sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR && sel < (int)tasks.size()) {
                    tasks.erase(tasks.begin() + sel);
                    RefreshListBox();
                    SaveTasks();
                }
            }
            break;

        case WM_CLOSE:
            SaveTasks();
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ToDoSidebar";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ToDoSidebar", L"To-Do Sidebar",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        100, 100, 220, 400,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}