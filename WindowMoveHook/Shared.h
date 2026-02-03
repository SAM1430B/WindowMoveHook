#pragma once
#include <windows.h>
#include <iostream>
#include <string>

struct Config {
    // General
    bool transmitTheWindow;
    int waitSeconds;
    bool getDimsThenSet;
    bool dontGetSize;
    bool dontGetPosition;

    int targetX, targetY, targetW, targetH;

    // FindWindow
    char reqWindowName[256];
    char reqClassName[256];

    // Hooks
    bool hookSetWindowPos;
    bool hookMoveWindow;
    bool hookAdjustWindowRect;
};

extern Config g_Config;
extern HWND g_hTargetWindow;

extern BOOL(WINAPI* TrueSetWindowPos)(HWND, HWND, int, int, int, int, UINT);
extern BOOL(WINAPI* TrueMoveWindow)(HWND, int, int, int, int, BOOL);
extern BOOL(WINAPI* TrueAdjustWindowRect)(LPRECT, DWORD, BOOL);

void LoadConfig();
HWND GetMainWindowHandle(DWORD targetPID, const char* requiredName, const char* requiredClass, DWORD timeoutMS);

BOOL WINAPI HookedSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
BOOL WINAPI HookedMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI HookedAdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);