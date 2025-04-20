#define UNICODE
#define _UNICODE

#include <windows.h>
#include <fstream>      // For file I/O (std::wifstream, std::wofstream)
#include <vector>       // For std::vector
#include <string>       // For std::wstring
#include <locale>       // For std::locale
#include <codecvt>      // For std::codecvt_utf8
#include <CommCtrl.h>   // Required for some common controls styles if needed later

#pragma comment(lib, "Comctl32.lib") // Link against the Common Controls library if using advanced controls later

// Define control IDs
#define ID_ADD      1
#define ID_REMOVE   2
#define ID_LIST     3
#define ID_OK       4 // Used in the custom input dialog

// Global variables
HWND hListBox;                  // Handle to the list box control
std::vector<std::wstring> tasks; // Vector to store the task strings

// --- File Operations ---

// Save tasks to file with UTF-8 encoding
void SaveTasks() {
    // Use std::wofstream for wide character output file stream
    std::wofstream file("tasks.txt", std::ios::out | std::ios::binary); // Use binary mode for consistency
    if (file.is_open()) {
        // Imbue the file stream with a locale that uses the UTF-8 codecvt facet
        file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t, 0x10FFFF, std::generate_header>));
        for (const auto& task : tasks) {
            file << task << L"\n";
        }
    }
}

// Load tasks from file with UTF-8 encoding
void LoadTasks() {
    // Use std::wifstream for wide character input file stream
    std::wifstream file("tasks.txt", std::ios::in | std::ios::binary); // Use binary mode for consistency
    tasks.clear(); // Clear existing tasks before loading
    if (file.is_open()) {
        // Imbue the file stream with a locale that uses the UTF-8 codecvt facet
        file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t, 0x10FFFF, std::consume_header>));
        std::wstring line;
        while (std::getline(file, line)) {
             if (!line.empty()) {
                 tasks.push_back(line);
             }
        }
    }
}

// --- ListBox Operations ---

// Refresh ListBox with updated tasks from the global 'tasks' vector
void RefreshListBox() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& task : tasks) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)task.c_str());
    }
}

// --- Custom Input Dialog ---

// Window Procedure for the custom input dialog
LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit;

    switch (msg) {
        case WM_CREATE:
            hEdit = CreateWindowW(L"EDIT", NULL,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  10, 10, 200, 25,
                                  hwnd, NULL,
                                  (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                                  NULL);

            CreateWindowW(L"BUTTON", L"OK",
                          WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                          75, 50, 50, 25,
                          hwnd, (HMENU)ID_OK,
                          (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                          NULL);
            SetFocus(hEdit);
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_OK) {
                wchar_t* buffer = (wchar_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                if (buffer) {
                    GetWindowTextW(hEdit, buffer, 256);
                }
                DestroyWindow(hwnd);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


// Function to create and show the custom modal input dialog
bool ShowInputDialog(HWND parent, wchar_t* buffer, int maxLen) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    const wchar_t CLASS_NAME[] = L"InputClass";

    WNDCLASSW wc = {};
    if (!GetClassInfoW(hInstance, CLASS_NAME, &wc)) {
        wc.lpfnWndProc   = InputProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
    }

    buffer[0] = L'\0';

    HWND hwnd = CreateWindowW(
        CLASS_NAME, L"New Task",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 130,
        parent, NULL, hInstance, NULL
    );

    if (!hwnd) {
        // Now MessageBox resolves to MessageBoxW because UNICODE is defined
        MessageBoxW(parent, L"Failed to create input dialog!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)buffer);
    EnableWindow(parent, FALSE);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};
    while (IsWindow(hwnd) && GetMessageW(&msg, NULL, 0, 0)) { // Use GetMessageW explicitly or rely on UNICODE define
        TranslateMessage(&msg);
        DispatchMessageW(&msg); // Use DispatchMessageW explicitly or rely on UNICODE define
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);

    return wcslen(buffer) > 0;
}

// --- Main Window ---

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            LoadTasks();
            hListBox = CreateWindowW(L"LISTBOX", NULL,
                                     WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                                     10, 10, 260, 300,
                                     hwnd, (HMENU)ID_LIST,
                                     (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                                     NULL);

            CreateWindowW(L"BUTTON", L"Add Task",
                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                          10, 320, 120, 30,
                          hwnd, (HMENU)ID_ADD,
                          (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                          NULL);

            CreateWindowW(L"BUTTON", L"Remove Selected",
                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                          150, 320, 120, 30,
                          hwnd, (HMENU)ID_REMOVE,
                          (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                          NULL);
            RefreshListBox();
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_ADD) {
                wchar_t taskBuffer[256] = {};
                if (ShowInputDialog(hwnd, taskBuffer, 256)) {
                    tasks.push_back(taskBuffer);
                    RefreshListBox();
                    SaveTasks();
                }
            } else if (LOWORD(wParam) == ID_REMOVE) {
                int index = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    tasks.erase(tasks.begin() + index);
                    RefreshListBox();
                    SaveTasks();
                } else {
                    // Now MessageBox resolves to MessageBoxW because UNICODE is defined
                    MessageBoxW(hwnd, L"Please select a task to remove.", L"No Selection", MB_OK | MB_ICONINFORMATION);
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

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// --- Entry Point ---

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const wchar_t MAIN_CLASS_NAME[] = L"ToDoAppClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = MAIN_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        // Now MessageBox resolves to MessageBoxW because UNICODE is defined
        MessageBoxW(NULL, L"Main Window Registration Failed!", L"Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0, MAIN_CLASS_NAME, L"Simple To-Do List", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 410,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        // Now MessageBox resolves to MessageBoxW because UNICODE is defined
        MessageBoxW(NULL, L"Main Window Creation Failed!", L"Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0) > 0) { // Use GetMessageW explicitly or rely on UNICODE define
        TranslateMessage(&msg);
        DispatchMessageW(&msg); // Use DispatchMessageW explicitly or rely on UNICODE define
    }

    return (int)msg.wParam;
}