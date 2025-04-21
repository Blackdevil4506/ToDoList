#define UNICODE
#define _UNICODE

// Target Windows 10 version 1809 for Dark Mode APIs
#define _WIN32_WINNT 0x0A00 // Windows 10
#include <windows.h>
#include <windowsx.h> // For GET_X_LPARAM, GET_Y_LPARAM if needed, and control macros
#include <CommCtrl.h>   // For ListView, DateTimePicker, InitCommonControlsEx
#include <Uxtheme.h>    // For Dark Mode Theme APIs (SetWindowTheme)
#include <dwmapi.h>     // For DWM Dark Mode title bar API (Ensure Windows SDK is installed and configured in your IDE/build system)
#include <fstream>
#include <vector>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>      // For formatting date string
#include <iomanip>      // For std::put_time / std::get_time (alternative date handling)
#include <cwchar>       // For wcsncpy_s
#include <stdexcept>    // For std::stoi exceptions

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Uxtheme.lib")
#pragma comment(lib, "Dwmapi.lib")


// --- Control IDs ---
#define ID_ADD              1
#define ID_REMOVE           2
#define ID_LISTVIEW         3
#define ID_OK               4 // Input Dialog OK
#define ID_CANCEL           5 // Input Dialog Cancel
#define ID_TASK_TEXT        6 // Input Dialog Edit Control
#define ID_DUE_DATE         7 // Input Dialog DateTimePicker
#define ID_TOGGLE_DARK_MODE 8
#define ID_TOGGLE_ONTOP     9

// --- Dark Mode Colors (Example) ---
const COLORREF DARK_MODE_BG = RGB(0x20, 0x20, 0x20);
const COLORREF DARK_MODE_TEXT = RGB(0xFF, 0xFF, 0xFF);
const COLORREF LIGHT_MODE_BG = GetSysColor(COLOR_WINDOW);
const COLORREF LIGHT_MODE_TEXT = GetSysColor(COLOR_WINDOWTEXT);


// --- Task Data Structure ---
struct Task {
    std::wstring text;
    std::wstring dueDate; // Store as string e.g., "YYYY-MM-DD" or empty
};

// --- Input Dialog Data Structure ---
// Used to pass data to the manually created modal dialog window procedure
struct InputDialogData {
    Task* pTask;        // Pointer to the task being added/edited
    INT_PTR* pResult;   // Pointer to store the dialog result (ID_OK or IDCANCEL)
};

// --- Global Variables ---
HWND hListView;               // Handle to the ListView control
HWND hBtnAdd, hBtnRemove, hBtnToggleDark, hBtnToggleOnTop; // Button Handles
std::vector<Task> tasks;        // Vector to store tasks
bool g_isDarkMode = false;
bool g_isAlwaysOnTop = false;
HBRUSH g_hbrDarkModeBackground = NULL; // Brush for dark background


// --- Forward Declarations ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputProc(HWND, UINT, WPARAM, LPARAM);
bool ShowInputDialog(HWND, Task&); // Now returns true if OK clicked, modifies Task reference
void RefreshListView();
void SaveTasks();
void LoadTasks();
void UpdateDarkModeState(HWND);
void ApplyDarkModeToWindow(HWND);
void UpdateAlwaysOnTopState(HWND);
std::wstring SystemTimeToString(const SYSTEMTIME& st);
bool StringToSystemTime(const std::wstring& s, SYSTEMTIME& st);


// --- Dark Mode Helper Functions ---
// Function to check if dark mode should be used (Needs linking to UxTheme.lib)
bool ShouldAppsUseDarkMode() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    // Newer setting (seems more reliable) HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\SystemUsesLightTheme == 0
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"SystemUsesLightTheme", RRF_RT_DWORD, nullptr, &value, &size) == ERROR_SUCCESS) {
        return value == 0;
    }
    // Fallback to older setting
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_DWORD, nullptr, &value, &size) == ERROR_SUCCESS) {
        return value == 0;
    }
    // Default
    return false;
}

// --- File Operations ---
void SaveTasks() {
    // Use std::locale for UTF-8 conversion
    std::wofstream file("tasks.txt"); // Opens in text mode by default
    if (file.is_open()) {
         // Setting locale AFTER opening might not work reliably on all compilers/libs for file encoding.
         // Better: Use OS-specific functions or libraries designed for UTF-8 file I/O if locale fails.
         // For MSVC, _wfopen_s with "w, ccs=UTF-8" is an option.
         // Let's stick to locale for portability attempt, but be aware of potential issues.
        try {
             file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
        } catch (const std::runtime_error& e) {
            // Handle locale setting error if needed (e.g., locale not supported)
             OutputDebugStringW(L"Warning: Could not set UTF-8 locale for saving tasks.\n");
        }

        for (const auto& task : tasks) {
            // Use tab as a simple separator
            // Replace potential tabs in the task text itself to avoid parsing issues
            std::wstring safeText = task.text;
            size_t pos = safeText.find(L'\t');
            while(pos != std::wstring::npos) {
                safeText.replace(pos, 1, L" "); // Replace tab with space
                pos = safeText.find(L'\t', pos + 1);
            }
            file << safeText << L"\t" << task.dueDate << L"\n";
        }
    } else {
         OutputDebugStringW(L"Error: Could not open tasks.txt for saving.\n");
    }
}

void LoadTasks() {
    // Use std::locale for UTF-8 conversion
    std::wifstream file("tasks.txt"); // Opens in text mode
    tasks.clear();
    if (file.is_open()) {
        try {
            file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
        } catch (const std::runtime_error& e) {
             OutputDebugStringW(L"Warning: Could not set UTF-8 locale for loading tasks.\n");
        }

        std::wstring line;
        while (std::getline(file, line)) {
            // Remove potential BOM if present (though codecvt_utf8 should handle it)
            // if (line.length() > 0 && line[0] == L'\xFEFF') {
            //     line.erase(0, 1);
            // }
            if (line.empty()) continue;

            Task task;
            size_t tabPos = line.find(L'\t');
            if (tabPos != std::wstring::npos) {
                task.text = line.substr(0, tabPos);
                task.dueDate = line.substr(tabPos + 1);
                 // Trim potential trailing newline/whitespace from dueDate
                 size_t endPos = task.dueDate.find_last_not_of(L" \n\r\t");
                 if(endPos != std::wstring::npos) {
                     task.dueDate = task.dueDate.substr(0, endPos + 1);
                 } else {
                      task.dueDate = L""; // Due date was only whitespace
                 }

            } else {
                task.text = line; // No due date found
                // Trim potential trailing newline/whitespace from text
                 size_t endPos = task.text.find_last_not_of(L" \n\r\t");
                 if(endPos != std::wstring::npos) {
                     task.text = task.text.substr(0, endPos + 1);
                 } else {
                      task.text = L""; // Line was only whitespace
                 }

                task.dueDate = L"";
                 if (task.text.empty()) continue; // Skip lines that are effectively empty
            }
            tasks.push_back(task);
        }
    } else {
        OutputDebugStringW(L"Info: tasks.txt not found or could not be opened for loading.\n");
    }
}


// --- ListView Operations ---
void InitListViewColumns(HWND hListViewCtrl) {
    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.cx = 160; // Width of the 1st column
    lvc.pszText = (LPWSTR)L"Task"; // Column header text
    ListView_InsertColumn(hListViewCtrl, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 90; // Width of the 2nd column
    lvc.pszText = (LPWSTR)L"Due Date"; // Column header text
    ListView_InsertColumn(hListViewCtrl, 1, &lvc);
}

void RefreshListView() {
    ListView_DeleteAllItems(hListView); // Clear existing items

    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT; // We are setting text

    for (int i = 0; i < static_cast<int>(tasks.size()); ++i) { // Use static_cast for size comparison
        lvi.iItem = i; // Row index

        // --- Set Task Text (Column 0) ---
        lvi.iSubItem = 0; // Column index
        // Create a temporary modifiable buffer if needed, though const_cast *might* work
        // std::wstring tempText = tasks[i].text; // Copy to ensure non-const buffer
        // lvi.pszText = &tempText[0];
        lvi.pszText = const_cast<wchar_t*>(tasks[i].text.c_str()); // Text for the first column
        if (ListView_InsertItem(hListView, &lvi) == -1) {
            // Handle error if needed
            OutputDebugStringW(L"Error: ListView_InsertItem failed.\n");
            continue;
        }

        // --- Set Due Date Text (Column 1) ---
        // std::wstring tempDate = tasks[i].dueDate; // Copy
        // lvi.pszText = &tempDate[0];
        lvi.pszText = const_cast<wchar_t*>(tasks[i].dueDate.c_str()); // Text for the second column
        ListView_SetItemText(hListView, i, 1, lvi.pszText); // Use ListView_SetItemText for subitems
    }
}


// --- Input Dialog Window Procedure (Manual Modal) ---
LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Retrieve the InputDialogData pointer stored in GWLP_USERDATA
    InputDialogData* pData = (InputDialogData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE: { // Use WM_NCCREATE to store data *before* WM_CREATE
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            InputDialogData* pInitData = (InputDialogData*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pInitData);
            // Let the message proceed to create the window
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            if (!pData || !pData->pTask || !pData->pResult) {
                // Should not happen if called correctly via ShowInputDialog
                OutputDebugStringW(L"Error: InputProc WM_CREATE received invalid data.\n");
                DestroyWindow(hwnd); // Abort dialog creation
                return -1; // Indicate failure
            }

            HINSTANCE hInstance = GetModuleHandle(NULL); // Or ((LPCREATESTRUCT)lParam)->hInstance

            // Create Controls inside the dialog window
            HWND hStaticTask = CreateWindowW(L"STATIC", L"Task:", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                                     10, 15, 80, 20, hwnd, NULL, hInstance, NULL);
            HWND hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   100, 10, 280, 25, hwnd, (HMENU)ID_TASK_TEXT, hInstance, NULL);

            HWND hStaticDate = CreateWindowW(L"STATIC", L"Due Date:", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                                     10, 55, 80, 20, hwnd, NULL, hInstance, NULL);
            HWND hDateTimePicker = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT | DTS_SHOWNONE,
                                              100, 50, 150, 25, hwnd, (HMENU)ID_DUE_DATE, hInstance, NULL);


            HWND hBtnOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                  100, 90, 80, 30, hwnd, (HMENU)ID_OK, hInstance, NULL);
            HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      200, 90, 80, 30, hwnd, (HMENU)ID_CANCEL, hInstance, NULL);


             // Initialize controls based on passed task data
            if (!pData->pTask->text.empty()) {
                SetWindowTextW(hEdit, pData->pTask->text.c_str());
            }
            if (!pData->pTask->dueDate.empty()) {
                SYSTEMTIME st = {};
                if (StringToSystemTime(pData->pTask->dueDate, st)) {
                    DateTime_SetSystemtime(hDateTimePicker, GDT_VALID, &st);
                } else {
                    DateTime_SetSystemtime(hDateTimePicker, GDT_NONE, NULL); // Set to "None" if parsing fails
                }
            } else {
                DateTime_SetSystemtime(hDateTimePicker, GDT_NONE, NULL); // Set to "None" for new or dateless tasks
            }

            // Apply dark mode to dialog and its controls if necessary
            if (g_isDarkMode) {
                 // Note: Basic WM_CTLCOLOR handling might not fully theme all controls.
                 // More advanced theming might require owner-draw or subclassing.
                 SetWindowTheme(hEdit, L"Explorer", NULL);
                 SetWindowTheme(hDateTimePicker, L"Explorer", NULL); // May or may not respect dark mode
                 SetWindowTheme(hBtnOk, L"Explorer", NULL);
                 SetWindowTheme(hBtnCancel, L"Explorer", NULL);
                 SetWindowTheme(hStaticTask, L"Explorer", NULL);
                 SetWindowTheme(hStaticDate, L"Explorer", NULL);
                 InvalidateRect(hwnd, NULL, TRUE); // Force redraw with theme/colors
            }

            SetFocus(hEdit);
            return 0; // Indicate success
        }

        case WM_COMMAND: {
             if (!pData || !pData->pTask || !pData->pResult) break; // Safety check

            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_OK: {
                    HWND hEdit = GetDlgItem(hwnd, ID_TASK_TEXT);
                    HWND hDateTimePicker = GetDlgItem(hwnd, ID_DUE_DATE);
                    int textLen = GetWindowTextLengthW(hEdit);
                    if (textLen > 0 && textLen < 512) { // Basic validation + buffer check
                         std::vector<wchar_t> buffer(textLen + 1);
                        GetWindowTextW(hEdit, buffer.data(), textLen + 1);
                        pData->pTask->text = buffer.data();

                        SYSTEMTIME st = {0};
                        LRESULT status = DateTime_GetSystemtime(hDateTimePicker, &st);

                        if (status == GDT_VALID) {
                            pData->pTask->dueDate = SystemTimeToString(st);
                        } else {
                            pData->pTask->dueDate = L""; // No date selected or invalid
                        }
                        *(pData->pResult) = ID_OK; // Set result pointer
                        DestroyWindow(hwnd); // Close the dialog
                    } else if (textLen == 0) {
                         MessageBoxW(hwnd, L"Task text cannot be empty.", L"Input Error", MB_OK | MB_ICONWARNING);
                    } else {
                        // Handle excessively long text if necessary, though unlikely with 512 limit here
                        MessageBoxW(hwnd, L"Task text is too long.", L"Input Error", MB_OK | MB_ICONWARNING);
                    }
                } break;

                case ID_CANCEL: {
                    *(pData->pResult) = IDCANCEL; // Set result pointer
                    DestroyWindow(hwnd); // Close the dialog
                } break;
            }
            return 0; // Handled
        }

        case WM_CLOSE: {
            if (pData && pData->pResult) {
                 *(pData->pResult) = IDCANCEL; // Treat close as Cancel
            }
            DestroyWindow(hwnd);
            return 0; // Handled
        }

        // Handle background colors for dark mode in the dialog
        // Note: This works best for static controls and button backgrounds. Edit controls
        // and complex controls like DateTimePicker might not respond fully.
       case WM_CTLCOLORDLG: // Dialog background
       case WM_CTLCOLORSTATIC: // Static text labels
       case WM_CTLCOLORBTN: {   // Buttons
            if (g_isDarkMode) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, DARK_MODE_TEXT);
                SetBkColor(hdc, DARK_MODE_BG);
                if (g_hbrDarkModeBackground == NULL) {
                    // Create brush on demand
                    g_hbrDarkModeBackground = CreateSolidBrush(DARK_MODE_BG);
                }
                // Ensure the brush is valid before returning
                 if (g_hbrDarkModeBackground) {
                    return (INT_PTR)g_hbrDarkModeBackground;
                 }
            }
            // If not dark mode or brush creation failed, fall through to default handling
        } break; // Needs break after case block

        // Handle Edit control colors (often needs specific handling)
        case WM_CTLCOLOREDIT: {
             if (g_isDarkMode) {
                 HDC hdc = (HDC)wParam;
                 SetTextColor(hdc, DARK_MODE_TEXT);
                 SetBkColor(hdc, RGB(0x30, 0x30, 0x30)); // Slightly lighter background for edit?
                 // Need a separate brush for edit background potentially
                 // For simplicity, reuse dark background brush, but could create another
                  if (g_hbrDarkModeBackground) return (INT_PTR)g_hbrDarkModeBackground;
             }
        } break;

        default:
            // Let the default window procedure handle other messages
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    // Default return for messages explicitly handled (like WM_COMMAND, WM_CLOSE)
    return 0;
}


// --- Show Input Dialog (Manual Modal Implementation) ---
// Returns true if user clicked OK, false otherwise. Task data is modified directly via reference.
bool ShowInputDialog(HWND parent, Task& taskData) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    const wchar_t CLASS_NAME[] = L"InputTaskDialogClass"; // Unique class name for the input dialog

    // Register the input dialog window class if not already registered
    WNDCLASSW wc = {};
    if (!GetClassInfoW(hInstance, CLASS_NAME, &wc)) {
        wc.lpfnWndProc   = InputProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = CLASS_NAME;
        // Background handled by WM_CTLCOLOR messages based on dark mode
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Default background
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        // wc.style = CS_HREDRAW | CS_VREDRAW; // Optional: Redraw on resize
        if (!RegisterClassW(&wc)) {
             MessageBoxW(parent, L"Failed to register input dialog class!", L"Error", MB_OK | MB_ICONERROR);
             return false;
        }
    }

    // Prepare data to pass to the dialog proc
    INT_PTR dialogResult = IDCANCEL; // Default result is Cancel
    InputDialogData data = {&taskData, &dialogResult};

    // Center the dialog relative to the parent window
    RECT rcParent, rcDialog;
    GetWindowRect(parent, &rcParent);
     int dialogWidth = 400;
     int dialogHeight = 170; // Adjusted height for controls
     int xPos = rcParent.left + (rcParent.right - rcParent.left - dialogWidth) / 2;
     int yPos = rcParent.top + (rcParent.bottom - rcParent.top - dialogHeight) / 2;


    // Create the modal dialog window
    HWND hwndInput = CreateWindowExW(
        WS_EX_DLGMODALFRAME,        // Dialog style border
        CLASS_NAME,
        (taskData.text.empty() ? L"Add New Task" : L"Edit Task"), // Dynamic title
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_POPUP, // Modal dialog styles
        xPos, yPos, dialogWidth, dialogHeight,
        parent,                     // Parent window
        NULL,                       // No menu
        hInstance,
        &data                       // Pass InputDialogData pointer as creation parameter
    );

    if (!hwndInput) {
        // Use MessageBoxW for UNICODE compatibility
        MessageBoxW(parent, L"Failed to create input dialog window!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Disable parent window (essential for modal behavior)
    EnableWindow(parent, FALSE);

    // --- Manual Modal Message Loop ---
    // Process messages until the dialog window is destroyed
    MSG msg = {};
    while (IsWindow(hwndInput) && GetMessageW(&msg, NULL, 0, 0)) {
         // IsDialogMessage checks for Tab, Enter, Esc keys for dialog navigation/actions
         // It might interfere if we manually handle OK/Cancel strictly via buttons.
         // Test if IsDialogMessage is needed or if direct button handling is sufficient.
         // If using IsDialogMessage, ensure buttons have correct IDs (IDOK, IDCANCEL)
         // or handle default actions appropriately.
        if (!IsDialogMessage(hwndInput, &msg)) { // Check if it's a navigation message for the dialog
             TranslateMessage(&msg);
             DispatchMessageW(&msg);
         }
    }
    // --- End of Message Loop ---

    // Re-enable parent window
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent); // Bring parent back to focus

    // The dialog window procedure (InputProc) should have set the 'dialogResult' value
    // before destroying the window (via the pointer in InputDialogData).
    return (dialogResult == ID_OK);
}


// --- Main Window Procedure ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Initialize Common Controls
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES; // Need ListView and DateTimePicker classes
            InitCommonControlsEx(&icex);

            HINSTANCE hInstance = GetModuleHandle(NULL); // Or use ((LPCREATESTRUCT)lParam)->hInstance

            // Create ListView
            hListView = CreateWindowExW(
                WS_EX_CLIENTEDGE, // Give it a border
                WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                10, 10, 260, 280, // Position and size (leave space for buttons)
                hwnd, (HMENU)ID_LISTVIEW, hInstance, NULL);

             if (!hListView) {
                 MessageBoxW(hwnd, L"Failed to create ListView!", L"Error", MB_ICONERROR | MB_OK);
                 return -1; // Fail window creation
             }


            // Set extended ListView styles for better appearance
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            // Add Columns to ListView
            InitListViewColumns(hListView);

            // Create Buttons
            int btnY = 300;
            int btnHeight = 30;
            int btnWidth = 125; // Slightly wider buttons
            int btnSpacing = 10;

            hBtnAdd = CreateWindowW(L"BUTTON", L"Add Task", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                10, btnY, btnWidth, btnHeight, hwnd, (HMENU)ID_ADD, hInstance, NULL);
            hBtnRemove = CreateWindowW(L"BUTTON", L"Remove Selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    10 + btnWidth + btnSpacing, btnY, btnWidth, btnHeight, hwnd, (HMENU)ID_REMOVE, hInstance, NULL);

            btnY += btnHeight + btnSpacing; // Move Y for next row of buttons
            hBtnToggleDark = CreateWindowW(L"BUTTON", L"Toggle Dark Mode", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        10, btnY, btnWidth, btnHeight, hwnd, (HMENU)ID_TOGGLE_DARK_MODE, hInstance, NULL);
            hBtnToggleOnTop = CreateWindowW(L"BUTTON", L"Always On Top: OFF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        10 + btnWidth + btnSpacing, btnY, btnWidth, btnHeight, hwnd, (HMENU)ID_TOGGLE_ONTOP, hInstance, NULL);

             // Check if buttons were created
            if (!hBtnAdd || !hBtnRemove || !hBtnToggleDark || !hBtnToggleOnTop) {
                 MessageBoxW(hwnd, L"Failed to create one or more buttons!", L"Error", MB_ICONERROR | MB_OK);
                 return -1; // Fail window creation
            }


            // Load tasks and populate ListView
            LoadTasks();
            RefreshListView();

             // Determine initial dark mode state from system or default to light
            g_isDarkMode = ShouldAppsUseDarkMode(); // Check system setting initially

            // Apply initial dark mode state TO THE WINDOW AND CONTROLS
            ApplyDarkModeToWindow(hwnd); // Set themes etc.
            UpdateDarkModeState(hwnd);   // Apply colors and DWM attributes based on g_isDarkMode

            // Apply initial always on top state
            UpdateAlwaysOnTopState(hwnd); // Set based on g_isAlwaysOnTop flag

        } break; // End WM_CREATE block


        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_ADD: {
                    Task newTask = {}; // Create an empty task
                    // Call the input dialog
                    if (ShowInputDialog(hwnd, newTask)) {
                        // OK was pressed, add the task
                        tasks.push_back(newTask);
                        RefreshListView();
                        SaveTasks();
                    }
                } break;

                case ID_REMOVE: {
                    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                    if (selectedIndex != -1 && selectedIndex < static_cast<int>(tasks.size())) { // Check bounds
                        tasks.erase(tasks.begin() + selectedIndex);
                        RefreshListView();
                        SaveTasks();
                    } else {
                        // Use MessageBoxW
                        MessageBoxW(hwnd, L"Please select a task to remove.", L"No Selection", MB_OK | MB_ICONINFORMATION);
                    }
                } break;

                case ID_TOGGLE_DARK_MODE:
                    g_isDarkMode = !g_isDarkMode; // Toggle the flag
                    UpdateDarkModeState(hwnd);    // Apply the change
                    break;

                case ID_TOGGLE_ONTOP:
                    g_isAlwaysOnTop = !g_isAlwaysOnTop; // Toggle the flag
                    UpdateAlwaysOnTopState(hwnd);      // Apply the change
                    // Update button text
                    SetWindowTextW(hBtnToggleOnTop, g_isAlwaysOnTop ? L"Always On Top: ON" : L"Always On Top: OFF");
                    break;
            }
        } break; // End WM_COMMAND block

        // Handle ListView notifications
        case WM_NOTIFY: {
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            if (lpnmh->idFrom == ID_LISTVIEW) {
                switch (lpnmh->code) {
                    case NM_DBLCLK: { // Double click to edit
                        LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
                        int index = lpnmia->iItem;
                        if (index >= 0 && index < static_cast<int>(tasks.size())) {
                            Task taskToEdit = tasks[index]; // Copy task data to edit
                            if (ShowInputDialog(hwnd, taskToEdit)) {
                                // OK was pressed, update the task
                                tasks[index] = taskToEdit; // Update task data in vector
                                RefreshListView();
                                SaveTasks();
                            }
                        }
                    } break;
                    // Add other notifications like LVN_KEYDOWN (e.g., DEL key to remove) if required
                     case LVN_KEYDOWN: {
                         LPNMLVKEYDOWN lpnmkd = (LPNMLVKEYDOWN)lParam;
                         if (lpnmkd->wVKey == VK_DELETE) {
                             int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                            if (selectedIndex != -1 && selectedIndex < static_cast<int>(tasks.size())) {
                                 // Optional: Ask for confirmation before deleting
                                 // if (MessageBoxW(hwnd, L"Are you sure you want to delete the selected task?", L"Confirm Deletion", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                                    tasks.erase(tasks.begin() + selectedIndex);
                                    RefreshListView();
                                    SaveTasks();
                                 //}
                             }
                         }
                     } break;
                }
            }
        } break; // End WM_NOTIFY block


        // Handle Dark Mode Theme Changes from System
        case WM_SETTINGCHANGE:
             // Check if the change relates to color scheme/theme
            if (lParam != 0 && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
                 // Update based on new system setting (optional: could keep manual toggle state)
                 g_isDarkMode = ShouldAppsUseDarkMode();
                 UpdateDarkModeState(hwnd);
            }
             // Also handle potential high contrast changes if desired
             // if (wParam == SPI_SETHIGHCONTRAST && lParam != 0) { ... }
            break;


        // Custom drawing for dark mode backgrounds (Main Window)
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            if (g_isDarkMode) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, DARK_MODE_TEXT);
                SetBkColor(hdc, DARK_MODE_BG);
                // Only create the brush once
                 if (g_hbrDarkModeBackground == NULL) {
                    g_hbrDarkModeBackground = CreateSolidBrush(DARK_MODE_BG);
                }
                // Return the brush for the background
                if (g_hbrDarkModeBackground) {
                     return (INT_PTR)g_hbrDarkModeBackground;
                 }
            }
             // If not dark mode or brush creation failed, fall through to default handling
             // Let DefWindowProc handle it for light mode or errors
              return DefWindowProc(hwnd, msg, wParam, lParam);

        } break; // End WM_CTLCOLOR* block for main window


        case WM_CLOSE:
            SaveTasks();
            // Clean up GDI brush if created
            if (g_hbrDarkModeBackground) {
                DeleteObject(g_hbrDarkModeBackground);
                g_hbrDarkModeBackground = NULL;
            }
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

// --- Apply Dark Mode Settings ---
void ApplyDarkModeToWindow(HWND hwnd) {
     // Set dark mode theme for common controls (works best on newer Win10/11)
     // Note: May interfere with WM_CTLCOLOR*. Experiment to see what works best.
     // Setting to "Explorer" is often a good starting point.
     SetWindowTheme(hListView, L"Explorer", NULL);
     SetWindowTheme(hBtnAdd, L"Explorer", NULL);
     SetWindowTheme(hBtnRemove, L"Explorer", NULL);
     SetWindowTheme(hBtnToggleDark, L"Explorer", NULL);
     SetWindowTheme(hBtnToggleOnTop, L"Explorer", NULL);

     // Forcing ListView text/background colors might be needed if themes don't fully work
     // if (g_isDarkMode) {
     //    ListView_SetTextColor(hListView, DARK_MODE_TEXT);
     //    ListView_SetTextBkColor(hListView, DARK_MODE_BG);
     //    ListView_SetBkColor(hListView, DARK_MODE_BG); // Background behind items
     // } else {
     //    ListView_SetTextColor(hListView, GetSysColor(COLOR_WINDOWTEXT));
     //    ListView_SetTextBkColor(hListView, GetSysColor(COLOR_WINDOW));
     //    ListView_SetBkColor(hListView, GetSysColor(COLOR_WINDOW));
     // }
}


void UpdateDarkModeState(HWND hwnd) {
    // Apply DWM attribute for title bar etc. (Win 10+)
    // Use DWMWA_USE_IMMERSIVE_DARK_MODE (20) for 20H1+
    // Use DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 (19) for older Win10 (optional fallback)
    // Need to link Dwmapi.lib
    BOOL useDarkModeForDwm = g_isDarkMode; // Use our toggle state
    // Ignore errors, it might fail on older systems or if DWM is off
    DwmSetWindowAttribute(hwnd, 20, &useDarkModeForDwm, sizeof(useDarkModeForDwm));
    // DwmSetWindowAttribute(hwnd, 19, &useDarkModeForDwm, sizeof(useDarkModeForDwm)); // Fallback if needed

    // Update the main window background brush
    if (g_hbrDarkModeBackground) { // Delete the old brush first
        DeleteObject(g_hbrDarkModeBackground);
        g_hbrDarkModeBackground = NULL;
    }
    HBRUSH hbrBackground = NULL;
    if (g_isDarkMode) {
        // Don't create here, WM_CTLCOLOR will create it on demand
        // g_hbrDarkModeBackground = CreateSolidBrush(DARK_MODE_BG);
        // hbrBackground = g_hbrDarkModeBackground;
         hbrBackground = CreateSolidBrush(DARK_MODE_BG); // Or create here and manage lifecycle carefully
         if (hbrBackground) { // Store the new brush handle if created
             g_hbrDarkModeBackground = hbrBackground;
         } else { // Fallback if brush creation fails
             hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
         }

    } else {
        hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Use default light mode background
    }
    // SetClassLongPtr might need a DeleteObject if replacing an existing brush handle
    // It's often safer to handle background in WM_ERASEBKGND or rely on WM_CTLCOLOR*
    // SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hbrBackground); // Update the class background brush


    // Re-apply themes to controls in case they need updating
    ApplyDarkModeToWindow(hwnd);

    // Trigger re-painting of the window and its children
    InvalidateRect(hwnd, NULL, TRUE); // Invalidate the whole window area
    UpdateWindow(hwnd); // Force WM_PAINT immediately

    // Explicitly redraw child controls to apply color changes from WM_CTLCOLOR*
    // RDW_INVALIDATE | RDW_ERASE forces repaint and background erase
    RedrawWindow(hListView, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
    RedrawWindow(hBtnAdd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
    RedrawWindow(hBtnRemove, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
    RedrawWindow(hBtnToggleDark, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
    RedrawWindow(hBtnToggleOnTop, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
}

// --- Apply Always On Top State ---
void UpdateAlwaysOnTopState(HWND hwnd) {
    SetWindowPos(hwnd,
        g_isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, // Ignored due to flags below
        SWP_NOMOVE | SWP_NOSIZE); // Don't change position or size
}

// --- Date/Time Helpers ---
std::wstring SystemTimeToString(const SYSTEMTIME& st) {
    wchar_t buffer[11]; // YYYY-MM-DD\0
    // Use swprintf_s for safety
    swprintf_s(buffer, sizeof(buffer)/sizeof(wchar_t), L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return std::wstring(buffer);
}

bool StringToSystemTime(const std::wstring& s, SYSTEMTIME& st) {
    // Basic parsing, assumes "YYYY-MM-DD" format
    if (s.length() != 10 || s[4] != L'-' || s[7] != L'-') {
        return false;
    }
    try {
        // Use std::stoi with boundary checks
        int year = std::stoi(s.substr(0, 4));
        int month = std::stoi(s.substr(5, 2));
        int day = std::stoi(s.substr(8, 2));

        // Basic validation (could be more robust checking days in month)
        if (year < 1601 || year > 30827 || // Valid SYSTEMTIME range approx
            month < 1 || month > 12 ||
            day < 1 || day > 31) {
            return false;
        }

        st.wYear = (WORD)year;
        st.wMonth = (WORD)month;
        st.wDay = (WORD)day;
        st.wDayOfWeek = 0; // Unknown
        st.wHour = 0;
        st.wMinute = 0;
        st.wSecond = 0;
        st.wMilliseconds = 0;
        return true;
    } catch (const std::invalid_argument&) {
        return false; // stoi failed (not a number)
    } catch (const std::out_of_range&) {
        return false; // stoi failed (out of range for int)
    } catch (...) {
        return false; // Other potential exceptions
    }
}


// --- Entry Point ---
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
// Use wWinMain for UNICODE entry point
    const wchar_t MAIN_CLASS_NAME[] = L"ToDoAppClassSidebar";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = MAIN_CLASS_NAME;
    // Background will be handled by WM_CTLCOLOR* / WM_ERASEBKGND based on dark mode state
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Default, maybe overridden
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION); // Load default app icon
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // Load standard arrow cursor

    if (!RegisterClassW(&wc)) {
        // Use MessageBoxW
        MessageBoxW(NULL, L"Main Window Registration Failed!", L"Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    // Create the main application window
    HWND hwnd = CreateWindowExW(
        0,                          // Optional window styles
        MAIN_CLASS_NAME,            // Window class
        L"Sidebar To-Do List",      // Window text
        WS_OVERLAPPEDWINDOW,        // Window style

        // Position and size
        CW_USEDEFAULT, CW_USEDEFAULT, 295, 420, // Adjusted size slightly

        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        // Use MessageBoxW
        MessageBoxW(NULL, L"Main Window Creation Failed!", L"Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0) > 0) { // Use GetMessageW for UNICODE
        TranslateMessage(&msg);
        DispatchMessageW(&msg); // Use DispatchMessageW for UNICODE
    }

    return (int)msg.wParam;
}

/*
Potential IntelliSense Error Note:
If you still see errors like "cannot open source file 'dwmapi.h'" or similar for Windows headers,
it means your IDE (like VS Code) or build environment is not correctly configured to find the
Windows SDK include paths.

To Fix (Example for VS Code C/C++ Extension):
1.  Ensure you have the Windows SDK installed (usually comes with Visual Studio Installer).
2.  Open your `.vscode/c_cpp_properties.json` file.
3.  Under the "includePath", make sure the path to the SDK's 'Include' directory (e.g.,
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22000.0/um" and
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22000.0/shared") is present.
    Replace "10.0.22000.0" with your installed SDK version.
4.  If using a different build system (CMake, Meson, Makefile), configure the include paths there.
5.  Restarting VS Code or reloading the C/C++ extension might be necessary.

The code itself includes the necessary headers and #pragma comments for linking, so the
remaining issues are likely environmental setup.
*/