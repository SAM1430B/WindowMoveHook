#include "Shared.h"

BOOL WINAPI HookedSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
    if (hWnd == g_hTargetWindow) {
        int finalX = g_Config.targetX;
        int finalY = g_Config.targetY;
        int finalW = (g_Config.targetW > 0) ? g_Config.targetW : cx;
        int finalH = (g_Config.targetH > 0) ? g_Config.targetH : cy;

        return TrueSetWindowPos(hWnd, hWndInsertAfter, finalX, finalY, finalW, finalH, uFlags);
    }
    return TrueSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

BOOL WINAPI HookedMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint) {
    if (hWnd == g_hTargetWindow) {
        int finalX = g_Config.targetX;
        int finalY = g_Config.targetY;
        int finalW = (g_Config.targetW > 0) ? g_Config.targetW : nWidth;
        int finalH = (g_Config.targetH > 0) ? g_Config.targetH : nHeight;

        return TrueMoveWindow(hWnd, finalX, finalY, finalW, finalH, bRepaint);
    }
    return TrueMoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}

BOOL WINAPI HookedAdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) {
    return TrueAdjustWindowRect(lpRect, dwStyle, bMenu);
}