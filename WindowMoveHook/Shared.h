#pragma once
#include <windows.h>
#include <windowsx.h> 
#include <string>
#include <vector>

struct Config {
    // General
    bool transmitTheWindow;
    int waitSeconds;
    bool getDimsThenSet;
    bool dontGetSize;
    bool dontGetPosition;

    int targetX, targetY, targetW, targetH;

    // Caller Filtering
    std::vector<std::string> bypassModules;

    // Fake Size (Internal Resolution)
    bool enableFakeSize;
    int fakeW, fakeH;

    // FindWindow
    char reqWindowName[256];
    char reqClassName[256];

    // Hooks
    bool hookSetWindowPos;
    bool hookMoveWindow;
    bool hookAdjustWindowRect;
    bool hookGetClientRect;
    bool hookScreenToClient;
    bool hookWndProc;
};

extern Config g_Config;
extern HWND g_hTargetWindow;

// Original function pointers
extern BOOL(WINAPI* TrueSetWindowPos)(HWND, HWND, int, int, int, int, UINT);
extern BOOL(WINAPI* TrueMoveWindow)(HWND, int, int, int, int, BOOL);
extern BOOL(WINAPI* TrueAdjustWindowRect)(LPRECT, DWORD, BOOL);
extern BOOL(WINAPI* TrueGetClientRect)(HWND, LPRECT);
extern BOOL(WINAPI* TrueScreenToClient)(HWND, LPPOINT);
extern int(WINAPI* TrueMapWindowPoints)(HWND, HWND, LPPOINT, UINT);

// Original WndProc pointer
extern WNDPROC OriginalWndProc;

void LoadConfig();
HWND GetMainWindowHandle(DWORD targetPID, const char* requiredName, const char* requiredClass, DWORD timeoutMS);
void ExpandToWindowSize(HWND hWnd, int clientW, int clientH, int& windowW, int& windowH);

// Hook definitions
BOOL WINAPI HookedSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
BOOL WINAPI HookedMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI HookedAdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
BOOL WINAPI HookedGetClientRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI HookedScreenToClient(HWND hWnd, LPPOINT lpPoint);
int WINAPI HookedMapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints);

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);