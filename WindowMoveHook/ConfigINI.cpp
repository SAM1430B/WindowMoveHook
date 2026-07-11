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

    // Coordinates (Physical Size)
    g_Config.targetX = GetPrivateProfileIntA("General", "PosX", -99999, configFile.c_str());
    g_Config.targetY = GetPrivateProfileIntA("General", "PosY", -99999, configFile.c_str());
    g_Config.targetW = GetPrivateProfileIntA("General", "Width", 0, configFile.c_str());
    g_Config.targetH = GetPrivateProfileIntA("General", "Height", 0, configFile.c_str());

    // Fake Size (Internal Resolution)
    g_Config.enableFakeSize = GetPrivateProfileIntA("FakeSize", "Enable", 0, configFile.c_str());
    g_Config.fakeW = GetPrivateProfileIntA("FakeSize", "FakeWidth", 1920, configFile.c_str());
    g_Config.fakeH = GetPrivateProfileIntA("FakeSize", "FakeHeight", 1080, configFile.c_str());

    int fakeSizeDefault = g_Config.enableFakeSize ? 1 : 0;

    // FindWindow
    GetPrivateProfileStringA("FindWindow", "WindowName", "", g_Config.reqWindowName, 256, configFile.c_str());
    GetPrivateProfileStringA("FindWindow", "ClassName", "", g_Config.reqClassName, 256, configFile.c_str());

    // Hooks
    g_Config.hookSetWindowPos = GetPrivateProfileIntA("Hooks", "SetWindowPos", 0, configFile.c_str());
    g_Config.hookMoveWindow = GetPrivateProfileIntA("Hooks", "MoveWindow", 0, configFile.c_str());
    g_Config.hookAdjustWindowRect = GetPrivateProfileIntA("Hooks", "AdjustWindowRect", 0, configFile.c_str());
    g_Config.hookGetClientRect = GetPrivateProfileIntA("Hooks", "GetClientRect", fakeSizeDefault, configFile.c_str());
    g_Config.hookScreenToClient = GetPrivateProfileIntA("Hooks", "ScreenToClient", fakeSizeDefault, configFile.c_str());
    g_Config.hookWndProc = GetPrivateProfileIntA("Hooks", "WndProc", fakeSizeDefault, configFile.c_str());

    // Bypass Module Parsing
    char rawModules[512] = { 0 };
    GetPrivateProfileStringA("Hooks", "BypassModule", "", rawModules, sizeof(rawModules), configFile.c_str());
    g_Config.bypassModules.clear();

    if (rawModules[0] != '\0') {
        std::string rawStr(rawModules);
        size_t start = 0;
        size_t end = rawStr.find(',');

        while (end != std::string::npos) {
            std::string mod = rawStr.substr(start, end - start);
            mod.erase(0, mod.find_first_not_of(" \t\r\n"));
            mod.erase(mod.find_last_not_of(" \t\r\n") + 1);

            if (!mod.empty()) g_Config.bypassModules.push_back(mod);

            start = end + 1;
            end = rawStr.find(',', start);
        }

        std::string lastMod = rawStr.substr(start);
        lastMod.erase(0, lastMod.find_first_not_of(" \t\r\n"));
        lastMod.erase(lastMod.find_last_not_of(" \t\r\n") + 1);
        if (!lastMod.empty()) g_Config.bypassModules.push_back(lastMod);
    }
}