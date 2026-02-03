#include "Shared.h"
#include <string>

void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    std::string rootPath = iniPath;
    size_t lastSlash = rootPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        rootPath = rootPath.substr(0, lastSlash + 1);
    }

    std::string configFile = rootPath + "WindowMoveHook.ini";

    // General
    g_Config.transmitTheWindow = GetPrivateProfileIntA("General", "TransmitTheWindow", 0, configFile.c_str());
    g_Config.waitSeconds = GetPrivateProfileIntA("General", "Wait", 0, configFile.c_str());

    // Logic Switches
    g_Config.getDimsThenSet = GetPrivateProfileIntA("General", "GetWindowDimensionsThenSet", 0, configFile.c_str());
    g_Config.dontGetSize = GetPrivateProfileIntA("General", "DontGetSize", 0, configFile.c_str());
    g_Config.dontGetPosition = GetPrivateProfileIntA("General", "DontGetPosition", 0, configFile.c_str());

    // Coordinates
    g_Config.targetX = GetPrivateProfileIntA("General", "PosX", 0, configFile.c_str());
    g_Config.targetY = GetPrivateProfileIntA("General", "PosY", 0, configFile.c_str());
    g_Config.targetW = GetPrivateProfileIntA("General", "Width", 0, configFile.c_str());
    g_Config.targetH = GetPrivateProfileIntA("General", "Height", 0, configFile.c_str());

    // FindWindow
    GetPrivateProfileStringA("FindWindow", "WindowName", "", g_Config.reqWindowName, 256, configFile.c_str());
    GetPrivateProfileStringA("FindWindow", "ClassName", "", g_Config.reqClassName, 256, configFile.c_str());

    // Hooks
    g_Config.hookSetWindowPos = GetPrivateProfileIntA("Hooks", "SetWindowPos", 0, configFile.c_str());
    g_Config.hookMoveWindow = GetPrivateProfileIntA("Hooks", "MoveWindow", 0, configFile.c_str());
    g_Config.hookAdjustWindowRect = GetPrivateProfileIntA("Hooks", "AdjustWindowRect", 0, configFile.c_str());
}