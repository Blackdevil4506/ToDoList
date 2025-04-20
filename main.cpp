#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

#define ID_ADD 1
#define ID_REMOVE 2
#define ID_LISTBOX 3

HWND hListBox;
std::vector<std::wstring> tasks;

// Save tasks to a file
void SaveTasks() {
    std::ofstream file("tasks.txt"); // Use narrow string
    for (const auto& task : tasks) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        file << converter.to_bytes(task) << "\n";
    }
}


// Load tasks from a file
void LoadTasks() {
    tasks.clear();
    std::ifstream file("tasks.txt");
    std::string line;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    while (std::getline(file, line)) {
        tasks.push_back(converter.from_bytes(line));
    }
}


// Refresh ListBox UI
void RefreshListBox() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& task : tasks)
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)task.c_str());
}

// Custom input window class for adding tasks
LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit;
    switch (msg) {
        case WM_CREATE:
            hEdit = CreateWindowW(L"EDIT", nullptr,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 10, 260, 25,
                hwnd, nullptr, nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE,
                110, 50, 60, 25,
                hwnd, (HMENU)IDOK, nullptr, nullptr);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                wchar_t* buffer = (wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                GetWindowTextW(hEdit, buffer, 256);
                EndDialog(hwnd, IDOK);
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Show input dialog using CreateDialog-like logic (no resources)
bool ShowInputDialog(HWND parent, wchar_t* buffer, int length) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = InputWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"InputWindow";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, L"InputWindow", L"Add Task",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 130,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)buffer);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!IsWindow(hwnd)) break; // Dialog closed
    }

    return wcslen(buffer) > 0;
}

// Main window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            LoadTasks();

            hListBox = CreateWindowW(L"LISTBOX", nullptr,
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
                10, 10, 260, 280,
                hwnd, (HMENU)ID_LISTBOX,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                nullptr);

            CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE,
                10, 300, 120, 30,
                hwnd, (HMENU)ID_ADD,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                nullptr);

            CreateWindowW(L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE,
                150, 300, 120, 30,
                hwnd, (HMENU)ID_REMOVE,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                nullptr);

            RefreshListBox();
            break;

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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ToDoSidebar";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ToDoSidebar", L"To-Do Sidebar",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        100, 100, 220, 400,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
