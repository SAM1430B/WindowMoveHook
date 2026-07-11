#include "Shared.h"
#include <detours.h> 

Config g_Config;
HWND g_hTargetWindow = nullptr;
WNDPROC OriginalWndProc = nullptr;

BOOL(WINAPI* TrueSetWindowPos)(HWND, HWND, int, int, int, int, UINT) = SetWindowPos;
BOOL(WINAPI* TrueMoveWindow)(HWND, int, int, int, int, BOOL) = MoveWindow;
BOOL(WINAPI* TrueAdjustWindowRect)(LPRECT, DWORD, BOOL) = AdjustWindowRect;
BOOL(WINAPI* TrueGetClientRect)(HWND, LPRECT) = GetClientRect;
BOOL(WINAPI* TrueScreenToClient)(HWND, LPPOINT) = ScreenToClient;
int(WINAPI* TrueMapWindowPoints)(HWND, HWND, LPPOINT, UINT) = MapWindowPoints;

typedef BOOL(WINAPI* PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT)(HANDLE);
typedef HRESULT(WINAPI* PFN_SET_PROCESS_DPI_AWARENESS)(int);
typedef BOOL(WINAPI* PFN_SET_PROCESS_DPI_AWARE)(VOID);

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif

void ForceDPIAwareness() {
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        auto setDpiCtx = (PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpiCtx) {
            setDpiCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    HMODULE hShcore = LoadLibraryA("Shcore.dll");
    if (hShcore) {
        auto setDpiAwareness = (PFN_SET_PROCESS_DPI_AWARENESS)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (setDpiAwareness) {
            setDpiAwareness(2);
            return;
        }
    }
    if (hUser32) {
        auto setDpiAware = (PFN_SET_PROCESS_DPI_AWARE)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (setDpiAware) setDpiAware();
    }
}

DWORD WINAPI InitializationThread(LPVOID lpParam) {
    ForceDPIAwareness();
    LoadConfig();

    if (g_Config.waitSeconds > 0) {
        Sleep(g_Config.waitSeconds * 1000);
    }

    DWORD pid = GetCurrentProcessId();
    g_hTargetWindow = GetMainWindowHandle(pid, g_Config.reqWindowName, g_Config.reqClassName, 5000);

    if (!g_hTargetWindow) return 0;

    // Subclass WndProc early
    if (g_Config.hookWndProc) {
        OriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
    }

    // Capture Logic
    if (g_Config.getDimsThenSet) {
        RECT rc;
        if (TrueGetClientRect(g_hTargetWindow, &rc)) {
            if (!g_Config.dontGetPosition) {
                RECT winRc;
                GetWindowRect(g_hTargetWindow, &winRc);
                g_Config.targetX = winRc.left;
                g_Config.targetY = winRc.top;
            }
            if (!g_Config.dontGetSize) {
                g_Config.targetW = rc.right - rc.left;
                g_Config.targetH = rc.bottom - rc.top;
            }
        }
    }

    // Transmit Logic (Apply initial move with border compensation)
    if (g_Config.transmitTheWindow) {
        UINT uFlags = SWP_NOZORDER | SWP_FRAMECHANGED;
        int windowW = g_Config.targetW;
        int windowH = g_Config.targetH;

        if (windowW <= 0 || windowH <= 0) {
            uFlags |= SWP_NOSIZE;
        }
        else {
            ExpandToWindowSize(g_hTargetWindow, g_Config.targetW, g_Config.targetH, windowW, windowH);
        }

        if (g_Config.targetX == -99999 || g_Config.targetY == -99999) {
            uFlags |= SWP_NOMOVE;
        }

        // Apply the physical window size
        TrueSetWindowPos(g_hTargetWindow, NULL,
            (g_Config.targetX == -99999) ? 0 : g_Config.targetX,
            (g_Config.targetY == -99999) ? 0 : g_Config.targetY,
            windowW, windowH, uFlags);

        // Force cross-thread window UI recalculation 
        PostMessage(g_hTargetWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        Sleep(500);
        PostMessage(g_hTargetWindow, WM_SYSCOMMAND, SC_RESTORE, 0);

        SetForegroundWindow(g_hTargetWindow);
        SetFocus(g_hTargetWindow);
    }

    // Install Detour Hooks
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (g_Config.hookSetWindowPos) DetourAttach(&(PVOID&)TrueSetWindowPos, HookedSetWindowPos);
    if (g_Config.hookMoveWindow) DetourAttach(&(PVOID&)TrueMoveWindow, HookedMoveWindow);
    if (g_Config.hookAdjustWindowRect) DetourAttach(&(PVOID&)TrueAdjustWindowRect, HookedAdjustWindowRect);
    if (g_Config.hookGetClientRect) DetourAttach(&(PVOID&)TrueGetClientRect, HookedGetClientRect);
    if (g_Config.hookScreenToClient) {
        DetourAttach(&(PVOID&)TrueScreenToClient, HookedScreenToClient);
        DetourAttach(&(PVOID&)TrueMapWindowPoints, HookedMapWindowPoints);
    }

    DetourTransactionCommit();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitializationThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        if (g_Config.hookSetWindowPos) DetourDetach(&(PVOID&)TrueSetWindowPos, HookedSetWindowPos);
        if (g_Config.hookMoveWindow) DetourDetach(&(PVOID&)TrueMoveWindow, HookedMoveWindow);
        if (g_Config.hookAdjustWindowRect) DetourDetach(&(PVOID&)TrueAdjustWindowRect, HookedAdjustWindowRect);
        if (g_Config.hookGetClientRect) DetourDetach(&(PVOID&)TrueGetClientRect, HookedGetClientRect);
        if (g_Config.hookScreenToClient) {
            DetourDetach(&(PVOID&)TrueScreenToClient, HookedScreenToClient);
            DetourDetach(&(PVOID&)TrueMapWindowPoints, HookedMapWindowPoints);
        }

        DetourTransactionCommit();

        if (OriginalWndProc && g_hTargetWindow) {
            SetWindowLongPtr(g_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)OriginalWndProc);
        }
        break;
    }
    return TRUE;
}