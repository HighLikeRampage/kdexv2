#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3dx9.h>
#include <atomic>

namespace Core {
    class VariablesClass {
    public:
        DWORD ProcIdFiveM;

        HWND g_hGameWindow;
        HWND g_hCheatWindow = nullptr;
        D3DXVECTOR2 g_vGameWindowSize;
        D3DXVECTOR2 g_vGameWindowPos;
        D3DXVECTOR2 g_vGameWindowCenter;

        std::string ServerIp;

        bool g_bPassedByThisVerify = false;
        bool g_Unload = false;
    };

    extern VariablesClass g_Variables;
    inline std::atomic<bool> g_AttachedToGame{false};
}
