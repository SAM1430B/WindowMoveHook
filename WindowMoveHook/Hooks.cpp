#include "Shared.h"
#include <intrin.h>
#include <algorithm>
#include <unordered_map>
#include <cmath>

#pragma intrinsic(_ReturnAddress)

std::unordered_map<HMODULE, bool> g_BypassCache;
SRWLOCK g_CacheLock = SRWLOCK_INIT;

// Verifies if the caller address belongs to a whitelisted overlay/tool module
bool IsCallerBypassed(void* returnAddress) {
    HMODULE hModule = NULL;

    // Get the module handle without changing its reference count
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)returnAddress, &hModule)) {

        AcquireSRWLockShared(&g_CacheLock);
        auto it = g_BypassCache.find(hModule);
        if (it != g_BypassCache.end()) {
            bool cachedResult = it->second;
            ReleaseSRWLockShared(&g_CacheLock);
            return cachedResult;
        }
        ReleaseSRWLockShared(&g_CacheLock);

        bool shouldBypass = false;
        char modulePath[MAX_PATH];

        if (GetModuleFileNameA(hModule, modulePath, MAX_PATH)) {
            std::string path(modulePath);
            size_t lastSlash = path.find_last_of("\\/");
            std::string fileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

            std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

            // Hardcoded permanent bypasses
            const std::vector<std::string> hardcodedModules = {
                "protoinput",
                "gtomnk",
                "screenshot input"
            };

            for (const std::string& hardcodedMod : hardcodedModules) {
                if (fileName.find(hardcodedMod) != std::string::npos) {
                    shouldBypass = true;
                    break;
                }
            }

            // User-defined bypasses from INI
            if (!shouldBypass && !g_Config.bypassModules.empty()) {
                for (std::string bypassMod : g_Config.bypassModules) {
                    std::transform(bypassMod.begin(), bypassMod.end(), bypassMod.begin(), ::tolower);
                    if (fileName.find(bypassMod) != std::string::npos) {
                        shouldBypass = true;
                        break;
                    }
                }
            }
        }

        AcquireSRWLockExclusive(&g_CacheLock);
        g_BypassCache[hModule] = shouldBypass;
        ReleaseSRWLockExclusive(&g_CacheLock);

        return shouldBypass;
    }
    return false;
}

// Converts desired Client Size into required Window Size (compensates for borders)
void ExpandToWindowSize(HWND hWnd, int clientW, int clientH, int& windowW, int& windowH) {
    RECT rc = { 0, 0, clientW, clientH };
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    AdjustWindowRectEx(&rc, dwStyle, GetMenu(hWnd) != NULL, dwExStyle);
    windowW = rc.right - rc.left;
    windowH = rc.bottom - rc.top;
}

BOOL WINAPI HookedSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
    if (hWnd == g_hTargetWindow) {
        int finalX = (g_Config.targetX == -99999) ? X : g_Config.targetX;
        int finalY = (g_Config.targetY == -99999) ? Y : g_Config.targetY;
        int clientW = (g_Config.targetW > 0) ? g_Config.targetW : cx;
        int clientH = (g_Config.targetH > 0) ? g_Config.targetH : cy;

        int windowW = clientW, windowH = clientH;
        if (g_Config.targetW > 0 && g_Config.targetH > 0) {
            ExpandToWindowSize(hWnd, clientW, clientH, windowW, windowH);
        }

        return TrueSetWindowPos(hWnd, hWndInsertAfter, finalX, finalY, windowW, windowH, uFlags);
    }
    return TrueSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

BOOL WINAPI HookedMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint) {
    if (hWnd == g_hTargetWindow) {
        int finalX = (g_Config.targetX == -99999) ? X : g_Config.targetX;
        int finalY = (g_Config.targetY == -99999) ? Y : g_Config.targetY;
        int clientW = (g_Config.targetW > 0) ? g_Config.targetW : nWidth;
        int clientH = (g_Config.targetH > 0) ? g_Config.targetH : nHeight;

        int windowW = clientW, windowH = clientH;
        if (g_Config.targetW > 0 && g_Config.targetH > 0) {
            ExpandToWindowSize(hWnd, clientW, clientH, windowW, windowH);
        }

        return TrueMoveWindow(hWnd, finalX, finalY, windowW, windowH, bRepaint);
    }
    return TrueMoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}

BOOL WINAPI HookedAdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) {
    return TrueAdjustWindowRect(lpRect, dwStyle, bMenu);
}

// Helper function to dynamically calculate the effective Fake Size
void GetEffectiveFakeSize(int realW, int realH, int& outFakeW, int& outFakeH) {
    if (g_Config.autoUpScale && realW > 0 && realH > 0) {
        if (g_Config.autoMaintainAspectRatio == 1) {
			// Maintain Aspect Ratio Mode: Scale up to meet minimums while preserving the original aspect ratio
            double scaleX = (realW < g_Config.autoUpScaleMinW) ? ((double)g_Config.autoUpScaleMinW / realW) : 1.0;
            double scaleY = (realH < g_Config.autoUpScaleMinH) ? ((double)g_Config.autoUpScaleMinH / realH) : 1.0;

            double scale = (std::max)(scaleX, scaleY);

            outFakeW = static_cast<int>(std::round(realW * scale));
            outFakeH = static_cast<int>(std::round(realH * scale));
        }
        else {
			// Independent Scaling Mode: Scale each dimension independently to meet minimums
            outFakeW = (std::max)(realW, g_Config.autoUpScaleMinW);
            outFakeH = (std::max)(realH, g_Config.autoUpScaleMinH);
        }
    }
    else {
        // Fallback to static FakeSize config
        outFakeW = g_Config.fakeW;
        outFakeH = g_Config.fakeH;
    }
}

// Fake Size Logic
BOOL WINAPI HookedGetClientRect(HWND hWnd, LPRECT lpRect) {
    void* retAddr = _ReturnAddress();
    BOOL result = TrueGetClientRect(hWnd, lpRect);

    if (result && g_Config.enableFakeSize && hWnd == g_hTargetWindow) {
        // Enforce fake size unless caller is bypassed
        if (!IsCallerBypassed(retAddr)) {
            int realW = lpRect->right - lpRect->left;
            int realH = lpRect->bottom - lpRect->top;

            int fakeW, fakeH;
            GetEffectiveFakeSize(realW, realH, fakeW, fakeH);

            lpRect->left = 0;
            lpRect->top = 0;
            lpRect->right = fakeW;
            lpRect->bottom = fakeH;
        }
    }
    return result;
}

void GetMouseScaleFactors(double& scaleX, double& scaleY) {
    if (!g_hTargetWindow) {
        scaleX = 1.0; scaleY = 1.0;
        return;
    }

    RECT realClient;
    if (TrueGetClientRect(g_hTargetWindow, &realClient)) {
        int realW = realClient.right - realClient.left;
        int realH = realClient.bottom - realClient.top;

        if (realW > 0 && realH > 0) {
            int fakeW, fakeH;
            GetEffectiveFakeSize(realW, realH, fakeW, fakeH);

            scaleX = (double)fakeW / realW;
            scaleY = (double)fakeH / realH;
            return;
        }
    }

    scaleX = 1.0; scaleY = 1.0;
}

// Mouse Correction: API Scaling
BOOL WINAPI HookedScreenToClient(HWND hWnd, LPPOINT lpPoint) {
    void* retAddr = _ReturnAddress();
    BOOL result = TrueScreenToClient(hWnd, lpPoint);

    if (result && g_Config.enableFakeSize && hWnd == g_hTargetWindow) {
        if (!IsCallerBypassed(retAddr)) {
            double scaleX, scaleY;
            GetMouseScaleFactors(scaleX, scaleY);

            // Scale the physical coordinate UP to the fake coordinate with pixel-perfect rounding
            lpPoint->x = static_cast<int>(std::round(lpPoint->x * scaleX));
            lpPoint->y = static_cast<int>(std::round(lpPoint->y * scaleY));
        }
    }
    return result;
}

// Mouse Correction: Multi-Point Inbound Scaling
int WINAPI HookedMapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints) {
    void* retAddr = _ReturnAddress();
    int result = TrueMapWindowPoints(hWndFrom, hWndTo, lpPoints, cPoints);

    // ONLY intercept INBOUND coordinates (From Screen/NULL to the Target Window)
    if (result != 0 && g_Config.enableFakeSize && hWndFrom == NULL && hWndTo == g_hTargetWindow) {
        if (!IsCallerBypassed(retAddr)) {
            double scaleX, scaleY;
            GetMouseScaleFactors(scaleX, scaleY);

            for (UINT i = 0; i < cPoints; ++i) {
                lpPoints[i].x = static_cast<int>(std::round(lpPoints[i].x * scaleX));
                lpPoints[i].y = static_cast<int>(std::round(lpPoints[i].y * scaleY));
            }
        }
    }

    return result;
}

// Mouse Correction: Window Message Scaling
LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Safety check in case OriginalWndProc hasn't finalized assignment yet
    if (!OriginalWndProc) return DefWindowProc(hWnd, uMsg, wParam, lParam);

    if (hWnd == g_hTargetWindow && uMsg == WM_GETMINMAXINFO) {
        LRESULT res = CallWindowProc(OriginalWndProc, hWnd, uMsg, wParam, lParam);

        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1;
        mmi->ptMinTrackSize.y = 1;

        return res;
    }

    if (g_Config.removeBorders && hWnd == g_hTargetWindow) {

        if (uMsg == WM_STYLECHANGING) {
            LPSTYLESTRUCT pStyle = (LPSTYLESTRUCT)lParam;
            if (wParam == GWL_STYLE) {
                pStyle->styleNew &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            }
            else if (wParam == GWL_EXSTYLE) {
                pStyle->styleNew &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
            }
        }

        if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) {
            DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);

            if (dwStyle & WS_CAPTION) {
                DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

                SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU));
                SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE));

                // Force Windows redraw the frame
                SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }
    }

    if (g_Config.enableFakeSize && hWnd == g_hTargetWindow) {

        if (uMsg == WM_MOUSEMOVE || uMsg == WM_MOUSEHOVER ||
            uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_LBUTTONDBLCLK ||
            uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_RBUTTONDBLCLK ||
            uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP || uMsg == WM_MBUTTONDBLCLK ||
            uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP || uMsg == WM_XBUTTONDBLCLK) {

            // Extract the original physical coordinates
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            // Dynamically get the exact scale
            double scaleX, scaleY;
            GetMouseScaleFactors(scaleX, scaleY);

            // Scale the physical coordinate up to the fake coordinate
            xPos = static_cast<int>(std::round(xPos * scaleX));
            yPos = static_cast<int>(std::round(yPos * scaleY));

            // Re-pack the modified coordinates safely back into a 32-bit lParam
            lParam = MAKELPARAM(xPos, yPos);
        }
    }

    return CallWindowProc(OriginalWndProc, hWnd, uMsg, wParam, lParam);
}