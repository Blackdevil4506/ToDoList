#include <windows.h>
#include <fstream>
#include <string>
#include <vector>

#define ID_ADD 1
#define ID_REMOVE 2
#define ID_LISTBOX 3

HWND hListBox;

std::vector<std::string> tasks;

void SaveTasks() {
    std::ofstream file("tasks.txt");
    for (const auto& task : tasks)
        file << task << "\n";
}

void LoadTasks() {
    tasks.clear();
    std::ifstream file("tasks.txt");
    std::string line;
    while (std::getline(file, line))
        tasks.push_back(line);
}

void RefreshListBox() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& task : tasks)
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)task.c_str());
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            hListBox = CreateWindow("LISTBOX", nullptr,
                WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
                10, 10, 180, 300, hwnd, (HMENU)ID_LISTBOX, nullptr, nullptr);

            CreateWindow("BUTTON", "Add", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                10, 320, 85, 30, hwnd, (HMENU)ID_ADD, nullptr, nullptr);

            CreateWindow("BUTTON", "Remove", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                105, 320, 85, 30, hwnd, (HMENU)ID_REMOVE, nullptr, nullptr);

            LoadTasks();
            RefreshListBox();
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_ADD) {
                char buffer[256] = {};
                if (DialogBoxParam(nullptr, MAKEINTRESOURCE(101), hwnd, [](HWND dlg, UINT msg, WPARAM wp, LPARAM lp) -> INT_PTR {
                    if (msg == WM_COMMAND && LOWORD(wp) == IDOK) {
                        GetDlgItemText(dlg, 1001, (LPSTR)lp, 256);
                        EndDialog(dlg, IDOK);
                        return TRUE;
                    }
                    if (msg == WM_COMMAND && LOWORD(wp) == IDCANCEL) {
                        EndDialog(dlg, IDCANCEL);
                        return TRUE;
                    }
                    return FALSE;
                }, (LPARAM)buffer) == IDOK) {
                    tasks.emplace_back(buffer);
                    RefreshListBox();
                    SaveTasks();
                }
            }
            else if (LOWORD(wParam) == ID_REMOVE) {
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ToDoSidebar";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "ToDoSidebar", "To-Do Sidebar",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        100, 100, 220, 400, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
