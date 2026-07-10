#include "Shared.h"
#include <string>
#include <algorithm>
#include <vector>

// Helper to safely convert text to lowercase and search for a substring
bool CaseInsensitiveContains(const char* fullText, const char* searchFor) {
    if (!fullText || !searchFor || searchFor[0] == '\0') return true;

    std::string textStr(fullText);
    std::string searchStr(searchFor);

    std::transform(textStr.begin(), textStr.end(), textStr.begin(), ::tolower);
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    return textStr.find(searchStr) != std::string::npos;
}

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

            RECT clientRect;
            if (GetClientRect(hWnd, &clientRect)) {
                if ((clientRect.right - clientRect.left) <= 1 || (clientRect.bottom - clientRect.top) <= 1) {
                    return TRUE;
                }
            }
            else {
                return TRUE;
            }

            char className[256] = { 0 };
            GetClassNameA(hWnd, className, sizeof(className));

            char windowName[256] = { 0 };
            GetWindowTextA(hWnd, windowName, sizeof(windowName));

            const std::vector<std::string> hardcodedIgnores = {
                "protoinput",
                "gtomnk",
                "screenshot input"
            };

            for (const std::string& ignoreWord : hardcodedIgnores) {
                if (CaseInsensitiveContains(windowName, ignoreWord.c_str()) ||
                    CaseInsensitiveContains(className, ignoreWord.c_str())) {
                    return TRUE;
                }
            }

            bool usingStrictFilters = (pData->reqName && pData->reqName[0]) ||
                (pData->reqClass && pData->reqClass[0]);

            if (usingStrictFilters) {
                if (pData->reqClass && pData->reqClass[0] != '\0') {
                    if (!CaseInsensitiveContains(className, pData->reqClass)) return TRUE;
                }

                if (pData->reqName && pData->reqName[0] != '\0') {
                    if (!CaseInsensitiveContains(windowName, pData->reqName)) return TRUE;
                }
            }
            else {
                // Filter out standard tool windows if no strict name is provided
                if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
                if (GetWindow(hWnd, GW_OWNER) != (HWND)0) return TRUE;
            }

            pData->bestHandle = hWnd;

            return TRUE;
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