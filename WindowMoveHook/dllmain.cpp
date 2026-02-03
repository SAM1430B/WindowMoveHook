#include "Shared.h"
#include <detours.h> 

Config g_Config;
HWND g_hTargetWindow = nullptr;

BOOL(WINAPI* TrueSetWindowPos)(HWND, HWND, int, int, int, int, UINT) = SetWindowPos;
BOOL(WINAPI* TrueMoveWindow)(HWND, int, int, int, int, BOOL) = MoveWindow;
BOOL(WINAPI* TrueAdjustWindowRect)(LPRECT, DWORD, BOOL) = AdjustWindowRect;

DWORD WINAPI InitializationThread(LPVOID lpParam) {
    LoadConfig();

    if (g_Config.waitSeconds > 0) {
        Sleep(g_Config.waitSeconds * 1000);
    }

    DWORD pid = GetCurrentProcessId();
    g_hTargetWindow = GetMainWindowHandle(pid, g_Config.reqWindowName, g_Config.reqClassName, 5000);

    if (!g_hTargetWindow) {
        OutputDebugStringA("[WindowMoveHook] Could not find target window.");
        return 0;
    }

    // Capture Logic
    if (g_Config.getDimsThenSet) {
        RECT rc;
        if (GetWindowRect(g_hTargetWindow, &rc)) {
            if (!g_Config.dontGetPosition) {
                g_Config.targetX = rc.left;
                g_Config.targetY = rc.top;
            }
            if (!g_Config.dontGetSize) {
                g_Config.targetW = rc.right - rc.left;
                g_Config.targetH = rc.bottom - rc.top;
            }
        }
    }

    // Transmit Logic (Apply initial move)
    if (g_Config.transmitTheWindow) {
        UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE;

        if (g_Config.targetW <= 0 || g_Config.targetH <= 0) {
            uFlags |= SWP_NOSIZE;
        }

        TrueSetWindowPos(g_hTargetWindow, NULL,
            g_Config.targetX, g_Config.targetY,
            g_Config.targetW, g_Config.targetH,
            uFlags);
    }

    // Install Hooks
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (g_Config.hookSetWindowPos) DetourAttach(&(PVOID&)TrueSetWindowPos, HookedSetWindowPos);
    if (g_Config.hookMoveWindow) DetourAttach(&(PVOID&)TrueMoveWindow, HookedMoveWindow);
    if (g_Config.hookAdjustWindowRect) DetourAttach(&(PVOID&)TrueAdjustWindowRect, HookedAdjustWindowRect);

    DetourTransactionCommit();
    return 0;
}

// DLL ENTRY POINT
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
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
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}