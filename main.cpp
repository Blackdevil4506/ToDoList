#include <windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <locale>
#include <codecvt>

#define ID_ADD    1
#define ID_REMOVE 2
#define ID_LIST   3
#define ID_OK     4

HWND hListBox;
std::vector<std::wstring> tasks;

// Save tasks to file with UTF-8 encoding
void SaveTasks() {
    std::wofstream file("tasks.txt", std::ios::out);
    file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    for (const auto& task : tasks)
        file << task << L"\n";
}

// Load tasks from file with UTF-8 encoding
void LoadTasks() {
    std::wifstream file("tasks.txt", std::ios::in);
    file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    std::wstring line;
    tasks.clear();
    while (std::getline(file, line))
        tasks.push_back(line);
}

// Refresh ListBox with updated tasks
void RefreshListBox() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& task : tasks) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)task.c_str());
    }
}

// Custom Input Dialog (as in the first program)
LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit;
    switch (msg) {
        case WM_CREATE:
            hEdit = CreateWindowW(L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
                                  10, 10, 200, 25, hwnd, NULL, NULL, NULL);
            CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE,
                          75, 50, 50, 25, hwnd, (HMENU)ID_OK, NULL, NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_OK) {
                wchar_t* buffer = (wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                GetWindowTextW(hEdit, buffer, 256);
                DestroyWindow(hwnd);
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Add task via custom input dialog
bool ShowInputDialog(HWND parent, wchar_t* buffer, int maxLen) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = InputProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"InputClass";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"InputClass", L"New Task",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                              CW_USEDEFAULT, CW_USEDEFAULT, 230, 130,
                              parent, NULL, wc.hInstance, NULL);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)buffer);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (IsWindow(hwnd) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return wcslen(buffer) > 0;
}

// Main window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            LoadTasks();
            hListBox = CreateWindowW(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                                     10, 10, 260, 300, hwnd, (HMENU)ID_LIST, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE,
                          10, 320, 120, 30, hwnd, (HMENU)ID_ADD, NULL, NULL);
            CreateWindowW(L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE,
                          150, 320, 120, 30, hwnd, (HMENU)ID_REMOVE, NULL, NULL);
            RefreshListBox(); // Ensure ListBox is populated
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_ADD) {
                wchar_t buf[256] = {};
                if (ShowInputDialog(hwnd, buf, 256)) {
                    tasks.push_back(buf);        // Add task to vector
                    RefreshListBox();            // Refresh the ListBox
                    SaveTasks();                 // Save tasks to file
                }
            } else if (LOWORD(wParam) == ID_REMOVE) {
                int index = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0); // Get selected index
                if (index != LB_ERR) {      // Ensure a valid item is selected
                    tasks.erase(tasks.begin() + index);   // Remove the selected task
                    RefreshListBox();        // Refresh ListBox after removal
                    SaveTasks();             // Save tasks to file
                }
            }
            break;
        case WM_CLOSE:
            SaveTasks(); // Save tasks on close
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ToDoAppClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, L"ToDoAppClass", L"To-Do Sidebar",
                                WS_OVERLAPPEDWINDOW, 100, 100, 300, 400,
                                NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
