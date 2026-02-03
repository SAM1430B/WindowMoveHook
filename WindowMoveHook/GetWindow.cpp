#include "Shared.h"

HWND GetMainWindowHandle(DWORD targetPID, const char* requiredName, const char* requiredClass, DWORD timeoutMS) {
    struct HandleData {
        DWORD pid;
        HWND bestHandle;
        const char* reqName;
        const char* reqClass;
    };

    ULONGLONG startTime = GetTickCount64();

    while (true) {
        HandleData data = { targetPID, nullptr, requiredName, requiredClass };

        auto EnumWindowsCallback = [](HWND hWnd, LPARAM lParam) -> BOOL {
            HandleData* pData = reinterpret_cast<HandleData*>(lParam);
            DWORD windowPID = 0;
            GetWindowThreadProcessId(hWnd, &windowPID);

            if (windowPID != pData->pid) return TRUE;
            if (!IsWindowVisible(hWnd)) return TRUE;

            bool usingStrictFilters = (pData->reqName && pData->reqName[0]) ||
                (pData->reqClass && pData->reqClass[0]);

            if (usingStrictFilters) {
                if (pData->reqClass && pData->reqClass[0] != '\0') {
                    char className[256];
                    GetClassNameA(hWnd, className, sizeof(className));
                    if (strcmp(className, pData->reqClass) != 0) return TRUE;
                }
                if (pData->reqName && pData->reqName[0] != '\0') {
                    char windowName[256];
                    GetWindowTextA(hWnd, windowName, sizeof(windowName));
                    if (strcmp(windowName, pData->reqName) != 0) return TRUE;
                }
            }
            else {
                if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
                if (GetWindow(hWnd, GW_OWNER) != (HWND)0) return TRUE;
            }

            pData->bestHandle = hWnd;
            return FALSE;
            };

        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));

        if (data.bestHandle != nullptr) {
            return data.bestHandle;
        }

        if ((GetTickCount64() - startTime) >= timeoutMS) {
            break;
        }
        Sleep(500);
    }
    return nullptr;
}