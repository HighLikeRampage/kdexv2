#include "Overlay.hpp"
#include "../../../Globals.hpp"
#include <d3d11.h>
#include <Windows.h>
#include <tchar.h>
#include <dwmapi.h>
#include <shellapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../../thirdparty/imgui/imgui.h"
#include "../../../thirdparty/imgui/backends/imgui_impl_win32.h"
#include "../../../thirdparty/imgui/backends/imgui_impl_dx11.h"
#include "../../../framework/settings/functions.h"
#include "../../../framework/data/fonts.h"
#include "../../../framework/data/images.h"
#include "../../../framework/data/GlowTopLeft.hpp"
#include "../../../framework/data/GlowBotRight.hpp"
#include "../../Core/Core.hpp"
#include "../../Core/Features/ESP.hpp"
#include "../../Core/Features/Exploits/Exploits.hpp"
#include "../../Security/Api/api.hpp"
#include "../../Security/AntiCrack.hpp"
#include "../../Security/UIAccess.hpp"
#include "../../Security/xorstr.hpp"
#include "../../../game/nvidia/nvidia_patch.hpp"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <filesystem>
#include "../../../framework/settings/search.h"
#include "../../../framework/settings/options_config.h"
#include "../../../framework/settings/dashboard_sync.h"
#include <psapi.h>
#pragma comment(lib, "psapi")

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();

void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::atomic<bool> g_dashboard_stop{ false };
static std::atomic<int> g_dashboard_validate_result{ -1 };
static std::mutex g_dashboard_mutex;
static std::string g_dashboard_token;
static std::vector<Security::Api::FivemCommandItem> g_dashboard_pending_commands;
static std::string g_pending_game_state_json;
static std::string g_pending_live_config_json;
static std::thread g_dashboard_thread;
static bool g_dashboard_thread_started = false;
static std::atomic<bool> s_unload_started{ false };
static HWINEVENTHOOK s_win_event_hook = nullptr;
static HWND s_hooked_game_hwnd = nullptr;
static volatile LONG s_game_window_moved = 0;
static volatile LONG s_dwm_changed = 0;
static HHOOK s_mouse_ll_hook = nullptr;
static HWND s_ll_overlay_hwnd = nullptr;
static std::atomic<bool> s_ll_eat_mouse{ false };


static std::atomic<int> s_raw_dx{ 0 };
static std::atomic<int> s_raw_dy{ 0 };
static std::atomic<bool> s_raw_input_registered{ false };
static POINT s_last_hw_pt    = { 0, 0 };
static bool  s_last_hw_valid = false;

static INT s_saved_mouse_params[3] = { 0, 0, 0 };
static std::atomic<bool> s_mouse_accel_saved{ false };

static void RestoreMouseAccelIfNeeded() {
    if (s_mouse_accel_saved.exchange(false)) {
        SystemParametersInfoA(SPI_SETMOUSE, 0, s_saved_mouse_params, 0);
    }
}

static void DisableMouseAccelForMenu() {
    if (s_mouse_accel_saved.load()) return;
    INT current[3] = { 0, 0, 0 };
    if (!SystemParametersInfoA(SPI_GETMOUSE, 0, current, 0)) return;
    if (current[2] == 0) return;
    s_saved_mouse_params[0] = current[0];
    s_saved_mouse_params[1] = current[1];
    s_saved_mouse_params[2] = current[2];
    s_mouse_accel_saved.store(true);
    INT no_accel[3] = { current[0], current[1], 0 };
    SystemParametersInfoA(SPI_SETMOUSE, 0, no_accel, 0);
}

static float s_virt_mouse_x = 0.f;
static float s_virt_mouse_y = 0.f;

static void RegisterOverlayRawInput(HWND hwnd, bool enable) {
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = enable ? RIDEV_INPUTSINK : RIDEV_REMOVE;
    rid.hwndTarget  = enable ? hwnd : nullptr;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    s_raw_input_registered.store(enable);
}

static LRESULT CALLBACK OverlayMouseLLProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION || !s_ll_eat_mouse.load())
        return CallNextHookEx(nullptr, nCode, wParam, lParam);

    MSLLHOOKSTRUCT* mh = (MSLLHOOKSTRUCT*)lParam;


    bool injected = (mh->flags & LLMHF_INJECTED) != 0;

    if (wParam == WM_MOUSEMOVE) {
        if (!injected) {
            if (s_last_hw_valid) {
                s_raw_dx.fetch_add((int)(mh->pt.x - s_last_hw_pt.x));
                s_raw_dy.fetch_add((int)(mh->pt.y - s_last_hw_pt.y));
            }
            s_last_hw_pt    = mh->pt;
            s_last_hw_valid = true;
        }

        return 1;
    }


    if (!ImGui::GetCurrentContext()) return 1;
    ImGuiIO& io = ImGui::GetIO();
    switch (wParam) {
        case WM_LBUTTONDOWN: io.AddMouseButtonEvent(0, true);  return 1;
        case WM_LBUTTONUP:   io.AddMouseButtonEvent(0, false); return 1;
        case WM_RBUTTONDOWN: io.AddMouseButtonEvent(1, true);  return 1;
        case WM_RBUTTONUP:   io.AddMouseButtonEvent(1, false); return 1;
        case WM_MBUTTONDOWN: io.AddMouseButtonEvent(2, true);  return 1;
        case WM_MBUTTONUP:   io.AddMouseButtonEvent(2, false); return 1;
        case WM_MOUSEWHEEL: {
            SHORT d = (SHORT)HIWORD(mh->mouseData);
            io.AddMouseWheelEvent(0.f, (float)d / (float)WHEEL_DELTA);
            return 1;
        }
        case WM_MOUSEHWHEEL: {
            SHORT d = (SHORT)HIWORD(mh->mouseData);
            io.AddMouseWheelEvent((float)d / (float)WHEEL_DELTA, 0.f);
            return 1;
        }
    }
    return 1;
}

void SubmitPendingGameState(const std::string& json) {
    if (json.empty()) return;
    std::lock_guard<std::mutex> lock(g_dashboard_mutex);
    g_pending_game_state_json = json;
}

void Gui::RequestEmergencyStop() {
    g_dashboard_stop = true;
}
void SubmitPendingLiveConfig(const std::string& json) {
    if (json.empty()) return;
    std::lock_guard<std::mutex> lock(g_dashboard_mutex);
    g_pending_live_config_json = json;
}

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#ifndef DWMWA_EXCLUDE_FROM_CAPTURE
#define DWMWA_EXCLUDE_FROM_CAPTURE 17
#endif

enum ZBID {
    ZBID_DEFAULT = 0,
    ZBID_DESKTOP = 1,
    ZBID_UIACCESS = 2,
    ZBID_IMMERSIVE_IHM = 3,
    ZBID_IMMERSIVE_NOTIFICATION = 4,
    ZBID_IMMERSIVE_APPCHROME = 5,
    ZBID_IMMERSIVE_MOGO = 6,
    ZBID_IMMERSIVE_EDGY = 7,
    ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
    ZBID_IMMERSIVE_INACTIVEDOCK = 9,
    ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
    ZBID_IMMERSIVE_ACTIVEDOCK = 11,
    ZBID_IMMERSIVE_BACKGROUND = 12,
    ZBID_IMMERSIVE_SEARCH = 13,
    ZBID_GENUINE_WINDOWS = 14,
    ZBID_IMMERSIVE_RESTRICTED = 15,
    ZBID_SYSTEM_TOOLS = 16,
    ZBID_LOCK = 17,
    ZBID_ABOVELOCK_UX = 18,
};

typedef HWND(WINAPI* tCreateWindowInBand)(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam,
    DWORD dwBand
);

static void CALLBACK OverlayWinEventProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
    InterlockedExchange(&s_game_window_moved, 1);
}

typedef struct {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} SP_USTR;

typedef struct {
    ULONG Flags;
    const SP_USTR* FullDllName;
    const SP_USTR* BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} SP_DLL_NOTIFICATION_DATA;

typedef VOID(CALLBACK* SP_DLL_NOTIFY_FN)(ULONG, const SP_DLL_NOTIFICATION_DATA*, PVOID);
typedef NTSTATUS(NTAPI* pfnLdrRegisterDllNotification)(ULONG, SP_DLL_NOTIFY_FN, PVOID, PVOID*);

static BYTE s_nvd_orig1[] = { 0x44, 0x8B, 0x82, 0x70, 0x01, 0x00, 0x00, 0x45, 0x85, 0xC0 };
static BYTE s_nvd_patch1[] = { 0x45, 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x45, 0x85, 0xC0 };
static BYTE s_nvd_orig2[] = { 0x8B, 0x88, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC9 };
static BYTE s_nvd_patch2[] = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC9 };
static BYTE s_nvd_orig3[] = { 0x44, 0x8B, 0x8A, 0x70, 0x01, 0x00, 0x00, 0x45, 0x85, 0xC9 };
static BYTE s_nvd_patch3[] = { 0x45, 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x45, 0x85, 0xC9 };
static BYTE s_nvd_orig4[] = { 0x8B, 0x80, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC0 };
static BYTE s_nvd_patch4[] = { 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC0 };
static BYTE s_nvd_orig5[] = { 0x8B, 0x81, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC0 };
static BYTE s_nvd_patch5[] = { 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC0 };
static BYTE s_nvd_orig6[] = { 0x8B, 0x83, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC0 };
static BYTE s_nvd_patch6[] = { 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC0 };
static BYTE s_nvd_orig7[] = { 0x8B, 0x89, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC9 };
static BYTE s_nvd_patch7[] = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC9 };
static BYTE s_nvd_orig8[] = { 0x8B, 0x8B, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC9 };
static BYTE s_nvd_patch8[] = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC9 };
static volatile LONG s_nvd_patched = 0;
static volatile LONG s_nvwgf2_patched = 0;
static PVOID s_dll_notification_cookie = nullptr;

static void PatchModulePatternAll(BYTE* base, SIZE_T size, const BYTE* search, const BYTE* replace, size_t len) {
    if (size < len) return;
    for (SIZE_T i = 0; i <= size - len; i++) {
        if (memcmp(base + i, search, len) == 0) {
            DWORD old;
            if (VirtualProtect(base + i, len, PAGE_EXECUTE_READWRITE, &old)) {
                memcpy(base + i, replace, len);
                VirtualProtect(base + i, len, old, &old);
                FlushInstructionCache(GetCurrentProcess(), base + i, len);
            }
        }
    }
}

static void PatchModulePattern(BYTE* base, SIZE_T size, BYTE* search, BYTE* replace, size_t len) {
    PatchModulePatternAll(base, size, search, replace, len);
}

static void PatchNvidiaModuleAllPatterns(BYTE* base, SIZE_T size) {
    PatchModulePatternAll(base, size, s_nvd_orig1, s_nvd_patch1, sizeof(s_nvd_orig1));
    PatchModulePatternAll(base, size, s_nvd_orig2, s_nvd_patch2, sizeof(s_nvd_orig2));
    PatchModulePatternAll(base, size, s_nvd_orig3, s_nvd_patch3, sizeof(s_nvd_orig3));
    PatchModulePatternAll(base, size, s_nvd_orig4, s_nvd_patch4, sizeof(s_nvd_orig4));
    PatchModulePatternAll(base, size, s_nvd_orig5, s_nvd_patch5, sizeof(s_nvd_orig5));
    PatchModulePatternAll(base, size, s_nvd_orig6, s_nvd_patch6, sizeof(s_nvd_orig6));
    PatchModulePatternAll(base, size, s_nvd_orig7, s_nvd_patch7, sizeof(s_nvd_orig7));
    PatchModulePatternAll(base, size, s_nvd_orig8, s_nvd_patch8, sizeof(s_nvd_orig8));
}

static void PatchNvidiaModuleByName(const char* dllName, volatile LONG* patched_flag) {
    if (InterlockedCompareExchange(patched_flag, 1, 0) != 0) return;
    HMODULE hMod = GetModuleHandleA(dllName);
    if (!hMod) { InterlockedExchange(patched_flag, 0); return; }
    MODULEINFO info = {};
    if (!GetModuleInformation(GetCurrentProcess(), hMod, &info, sizeof(info))) {
        InterlockedExchange(patched_flag, 0);
        return;
    }
    PatchNvidiaModuleAllPatterns((BYTE*)info.lpBaseOfDll, info.SizeOfImage);
}

static void PatchNvd3dumx() {
    PatchNvidiaModuleByName(xorstr("nvd3dumx.dll"), &s_nvd_patched);
    PatchNvidiaModuleByName(xorstr("nvwgf2umx.dll"), &s_nvwgf2_patched);
}

static void DisableNvidiaOverlayInjection() {
    typedef BOOL(WINAPI* pfnSetProcessMitigationPolicy)(int, PVOID, SIZE_T);
    HMODULE hK32 = GetModuleHandleA(xorstr("kernel32.dll"));
    if (!hK32) return;
    auto pSet = (pfnSetProcessMitigationPolicy)GetProcAddress(hK32, xorstr("SetProcessMitigationPolicy"));
    if (!pSet) return;
    DWORD extension_disable = 1;
    pSet(5, &extension_disable, sizeof(extension_disable));
}

static void WriteAbsoluteJump(void* target, void* detour) {
    DWORD old;
    VirtualProtect(target, 14, PAGE_EXECUTE_READWRITE, &old);
    BYTE* p = (BYTE*)target;
    p[0] = 0x48; p[1] = 0xB8;
    memcpy(p + 2, &detour, 8);
    p[10] = 0xFF; p[11] = 0xE0;
    p[12] = 0x90; p[13] = 0x90;
    VirtualProtect(target, 14, old, &old);
    FlushInstructionCache(GetCurrentProcess(), target, 14);
}

static BOOL WINAPI HookedGetWindowDisplayAffinity(HWND, DWORD* pwdAffinity) {
    *pwdAffinity = WDA_NONE;
    return TRUE;
}

static VOID CALLBACK NvDllLoadCallback(ULONG reason, const SP_DLL_NOTIFICATION_DATA* data, PVOID) {
    if (reason != 1 || !data || !data->BaseDllName || !data->BaseDllName->Buffer) return;
    USHORT nameChars = data->BaseDllName->Length / sizeof(WCHAR);
    const wchar_t* nvdll1 = L"nvd3dumx.dll";
    const wchar_t* nvdll2 = L"nvwgf2umx.dll";
    if (nameChars == wcslen(nvdll1) && _wcsnicmp(data->BaseDllName->Buffer, nvdll1, nameChars) == 0) {
        InterlockedExchange(&s_nvd_patched, 0);
        PatchNvd3dumx();
    } else if (nameChars == wcslen(nvdll2) && _wcsnicmp(data->BaseDllName->Buffer, nvdll2, nameChars) == 0) {
        InterlockedExchange(&s_nvwgf2_patched, 0);
        PatchNvd3dumx();
    }
}

static void InstallNvidiaShadowplayFix() {
    extern bool g_IsInjectedDll;
    if (!g_IsInjectedDll) {
        DisableNvidiaOverlayInjection();
    }
    PatchNvd3dumx();
    void* pGetWDA = (void*)GetProcAddress(GetModuleHandleA(xorstr("user32.dll")), xorstr("GetWindowDisplayAffinity"));
    if (pGetWDA) WriteAbsoluteJump(pGetWDA, (void*)HookedGetWindowDisplayAffinity);
    auto pLdrReg = (pfnLdrRegisterDllNotification)GetProcAddress(
        GetModuleHandleA(xorstr("ntdll.dll")), xorstr("LdrRegisterDllNotification"));
    if (pLdrReg) pLdrReg(0, NvDllLoadCallback, nullptr, &s_dll_notification_cookie);
}

namespace Gui {

    static HRESULT LoadTextureFromMemoryD3D11(ID3D11Device* pDevice, const void* pSrcData, SIZE_T SrcDataSize, ID3D11ShaderResourceView** ppSRV)
    {
        if (!pDevice || !pSrcData || SrcDataSize == 0 || !ppSRV) return E_INVALIDARG;
        *ppSRV = nullptr;

        typedef HRESULT(WINAPI* PFN_D3DX11)(ID3D11Device*, LPCVOID, SIZE_T, void*, void*, ID3D11ShaderResourceView**, HRESULT*);
        static PFN_D3DX11 s_pfnD3DX11 = nullptr;
        static bool s_checked = false;
        if (!s_checked) {
            s_checked = true;
            HMODULE hMod = LoadLibraryA(xorstr("d3dx11_43.dll"));
            if (hMod) s_pfnD3DX11 = (PFN_D3DX11)GetProcAddress(hMod, xorstr("D3DX11CreateShaderResourceViewFromMemory"));
        }
        if (s_pfnD3DX11)
            return s_pfnD3DX11(pDevice, pSrcData, SrcDataSize, nullptr, nullptr, ppSRV, nullptr);

        return E_NOTIMPL;
    }

    void PerformRestoreAll()
    {
        try {
            Core::Features::Exploits::RestoreAll();
        }
        catch (...) {}
    }

    void Overlay::Render()
    {
        static std::atomic<bool> s_attach_thread_running{ false };
        static std::wstring s_className, s_windowTitle;
        if (s_className.empty()) {
            s_className = AntiCrack::WindowCheck::widen((xorstr("ImGui Example")));
            s_windowTitle = AntiCrack::WindowCheck::widen((xorstr("Rotten Overlay")));
        }
        nvidia::runPatch();
        HINSTANCE hWndInst = g_hInstance ? g_hInstance : (HINSTANCE)GetModuleHandleW(NULL);
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hWndInst, nullptr, nullptr, nullptr, nullptr, s_className.c_str(), nullptr };
        ::RegisterClassExW(&wc);

        HWND game_hwnd = Utils::FindFiveMWindow();
        int x = 0, y = 0, w = 1920, h = 1080;

        if (option->param.second_monitor_display)
        {
            auto enum_proc = [](HMONITOR hMon, HDC, LPRECT, LPARAM lp) -> BOOL {
                std::vector<MONITORINFOEXW>* list = reinterpret_cast<std::vector<MONITORINFOEXW>*>(lp);
                MONITORINFOEXW info;
                info.cbSize = sizeof(info);
                if (GetMonitorInfoW(hMon, &info))
                    list->push_back(info);
                return TRUE;
            };
            std::vector<MONITORINFOEXW> monitors;
            EnumDisplayMonitors(nullptr, nullptr, enum_proc, reinterpret_cast<LPARAM>(&monitors));

            std::sort(monitors.begin(), monitors.end(), [](const MONITORINFOEXW& a, const MONITORINFOEXW& b) {
                if (a.rcMonitor.left != b.rcMonitor.left) return a.rcMonitor.left < b.rcMonitor.left;
                return a.rcMonitor.top < b.rcMonitor.top;
            });

            if (!monitors.empty())
            {
                int target_index = option->param.display_monitor_index;
                if (target_index < 0 || target_index >= (int)monitors.size())
                {
                    target_index = -1;
                    for (int i = 0; i < (int)monitors.size(); ++i)
                        if (!(monitors[i].dwFlags & MONITORINFOF_PRIMARY)) { target_index = i; break; }
                    if (target_index == -1) target_index = 0;
                }

                RECT r = monitors[target_index].rcMonitor;
                x = r.left;
                y = r.top;
                w = r.right - r.left;
                h = r.bottom - r.top;
            }
            else
            {
                w = GetSystemMetrics(SM_CXSCREEN);
                h = GetSystemMetrics(SM_CYSCREEN);
            }
        }
        else if (game_hwnd) {
            RECT game_rect;
            GetWindowRect(game_hwnd, &game_rect);
            x = game_rect.left;
            y = game_rect.top;
            w = game_rect.right - game_rect.left;
            h = game_rect.bottom - game_rect.top;

            if (w <= 0) w = 1920;
            if (h <= 0) h = 1080;
        } else {
            w = GetSystemMetrics(SM_CXSCREEN);
            h = GetSystemMetrics(SM_CYSCREEN);
        }

        bool uia_at_init = UIAccess::HasUIAccess();
        printf("[Overlay] init: uiAccess=%d, pid=%lu, rect=(%d,%d %dx%d)\n", (int)uia_at_init, GetCurrentProcessId(), x, y, w, h);
        fflush(stdout);

        HWND hwnd = nullptr;
        static auto pCreateWindowInBand = reinterpret_cast<tCreateWindowInBand>(GetProcAddress(GetModuleHandleA(xorstr("user32.dll")), xorstr("CreateWindowInBand")));
        printf("[Overlay] CreateWindowInBand ptr=%p\n", pCreateWindowInBand); fflush(stdout);

        if (pCreateWindowInBand) {
            hwnd = pCreateWindowInBand(
                WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                wc.lpszClassName,
                s_windowTitle.c_str(),
                WS_POPUP,
                x, y, w, h,
                nullptr, nullptr, wc.hInstance, nullptr,
                ZBID_SYSTEM_TOOLS
            );
            printf("[Overlay] CreateWindowInBand(ZBID_SYSTEM_TOOLS) hwnd=%p err=%lu\n", hwnd, GetLastError()); fflush(stdout);
            if (!hwnd) {
                hwnd = pCreateWindowInBand(
                    WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                    wc.lpszClassName,
                    s_windowTitle.c_str(),
                    WS_POPUP,
                    x, y, w, h,
                    nullptr, nullptr, wc.hInstance, nullptr,
                    ZBID_UIACCESS
                );
                printf("[Overlay] CreateWindowInBand(ZBID_UIACCESS) hwnd=%p err=%lu\n", hwnd, GetLastError()); fflush(stdout);
            }
        }

        if (!hwnd) {
            hwnd = ::CreateWindowExW(
                WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                wc.lpszClassName,
                s_windowTitle.c_str(),
                WS_POPUP,
                x, y, w, h,
                nullptr, nullptr, wc.hInstance, nullptr
            );
            printf("[Overlay] fallback CreateWindowExW hwnd=%p err=%lu (NO privileged band -- will NOT render over FSE)\n", hwnd, GetLastError()); fflush(stdout);
        }

        AntiCrack::SetOurOverlayHwnd(hwnd);

        s_ll_overlay_hwnd = hwnd;

        MARGINS margins = { -1 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

        InstallNvidiaShadowplayFix();

        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return;
        }
        if (option->param.second_monitor_display)
        {
            ::MoveWindow(hwnd, x, y, w, h, TRUE);
        }

        if (option->param.stream_proof) {
            BOOL exclude = TRUE;


        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.ConfigDebugHighlightIdConflicts = false;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        io.BackendFlags &= ~ImGuiBackendFlags_HasSetMousePos;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        ImFontConfig font_config;
        font_config.PixelSnapH = false;
        font_config.OversampleH = 2;
        font_config.OversampleV = 2;
        font_config.RasterizerMultiply = 1.0f;
        font_config.FontDataOwnedByAtlas = false;

        static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0400, 0x052F, 0x2DE0, 0x2DFF, 0xA640, 0xA69F, 0xE000, 0xF8FF, 0 };

        var->font.instrument_medium[0] = io.Fonts->AddFontFromMemoryTTF(instrument_sans_medium, sizeof(instrument_sans_medium), 16.0f, &font_config, ranges);
        var->font.instrument_medium[1] = io.Fonts->AddFontFromMemoryTTF(instrument_sans_medium, sizeof(instrument_sans_medium), 18.0f, &font_config, ranges);
        var->font.instrument_bold[0] = io.Fonts->AddFontFromMemoryTTF(instrument_sans_bold, sizeof(instrument_sans_bold), 20.0f, &font_config, ranges);

        static const ImWchar kdex_ranges[] = {
            0xE19B, 0xE19B,
            0xF002, 0xF002,
            0xF013, 0xF013,
            0xF017, 0xF017,
            0xF02B, 0xF02B,
            0xF03A, 0xF03A,
            0xF044, 0xF044,
            0xF04B, 0xF04B,
            0xF04D, 0xF04D,
            0xF05B, 0xF05B,
            0xF06E, 0xF06E,
            0xF07B, 0xF07B,
            0xF0AC, 0xF0AC,
            0xF0AD, 0xF0AD,
            0xF0C0, 0xF0C0,
            0xF0C5, 0xF0C5,
            0xF0E7, 0xF0E7,
            0xF11C, 0xF11C,
            0xF120, 0xF121,
            0xF188, 0xF188,
            0xF1EB, 0xF1EB,
            0xF2B5, 0xF2B5,
            0,
        };

        var->font.icons[0] = io.Fonts->AddFontFromMemoryTTF(kdex_icons, sizeof(kdex_icons), 16.0f, &font_config, kdex_ranges);
        var->font.icons[1] = io.Fonts->AddFontFromMemoryTTF(kdex_icons, sizeof(kdex_icons), 18.0f, &font_config, kdex_ranges);
        var->font.icons[2] = io.Fonts->AddFontFromMemoryTTF(kdex_icons, sizeof(kdex_icons), 20.0f, &font_config, kdex_ranges);
        var->font.icons[3] = io.Fonts->AddFontFromMemoryTTF(kdex_icons, sizeof(kdex_icons), 24.0f, &font_config, kdex_ranges);
        var->font.icons[4] = io.Fonts->AddFontFromMemoryTTF(kdex_icons, sizeof(kdex_icons), 28.0f, &font_config, kdex_ranges);

        ImFont* defaultFont = io.Fonts->AddFontDefault();
        for (int i = 0; i < 4; i++) var->font.brains_mono[i] = defaultFont;
        var->font.code = defaultFont;

        if (g_pd3dDevice) {
            ID3D11ShaderResourceView* texture = nullptr;
            HRESULT hr = LoadTextureFromMemoryD3D11(g_pd3dDevice, logo, sizeof(logo), &texture);
            if (SUCCEEDED(hr)) {
                var->window.logo_texture = (ImTextureID)texture;
            }

            hr = LoadTextureFromMemoryD3D11(g_pd3dDevice, banner, sizeof(banner), &texture);
            if (SUCCEEDED(hr)) {
                var->window.banner_texture = (ImTextureID)texture;
            }

            hr = LoadTextureFromMemoryD3D11(g_pd3dDevice, TopLeftCornerGlow, sizeof(TopLeftCornerGlow), &texture);
            if (SUCCEEDED(hr)) {
                var->window.top_left_glow_texture = (ImTextureID)texture;
            }

            hr = LoadTextureFromMemoryD3D11(g_pd3dDevice, BotRightCornerGlow, sizeof(BotRightCornerGlow), &texture);
            if (SUCCEEDED(hr)) {
                var->window.bot_right_glow_texture = (ImTextureID)texture;
            }
        }

        bool done = false;
        using auth_screen = decltype(var->auth)::screen;
        static bool menu_open = false;
        static int s_frames_rendered = 0;

        static HWND s_cached_game_hwnd = nullptr;
        static double s_last_hwnd_check = 0.0;
        static bool s_last_stream_proof = false;
        static bool s_last_menu_open = true;
        static bool s_last_game_active = false;
        static double s_last_rect_sync = 0.0;

        auto start_dashboard_thread = [&]() {
            if (g_dashboard_thread_started) return;
            g_dashboard_thread_started = true;
            g_dashboard_stop = false;
            std::thread([]() {
                int tick_count = 0;
                while (!g_dashboard_stop && !Core::g_Variables.g_Unload) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    if (g_dashboard_stop || Core::g_Variables.g_Unload) break;
                    std::string token;
                    std::string game_state_json;
                    std::string live_config_json;
                    {
                        std::lock_guard<std::mutex> lock(g_dashboard_mutex);
                        token = g_dashboard_token;
                        if (tick_count % 6 == 0 && !g_pending_game_state_json.empty()) {
                            game_state_json = std::move(g_pending_game_state_json);
                            g_pending_game_state_json.clear();
                        }
                        if (tick_count % 10 == 0 && !g_pending_live_config_json.empty()) {
                            live_config_json = std::move(g_pending_live_config_json);
                            g_pending_live_config_json.clear();
                        }
                    }
                    if (!token.empty()) {
                        int r = Security::Api::validate_session(token);
                        g_dashboard_validate_result.store(r);
                        if (tick_count % 4 == 0) {
                            auto cmds = Security::Api::fivem_fetch_commands(token);
                            if (!cmds.empty()) {
                                std::lock_guard<std::mutex> lock(g_dashboard_mutex);
                                for (auto& c : cmds) {
                                    if (c.type == std::string(xorstr("apply_live_config"))) {
                                        std::string live_data = Security::Api::live_config_get(token);
                                        if (!live_data.empty()) {
                                            c.payload = nlohmann::json::parse(live_data);
                                        }
                                    }
                                    g_dashboard_pending_commands.push_back(std::move(c));
                                }
                            }
                        }
                        if (tick_count % 8 == 0) {
                            Security::Api::fivem_set_logged(token, true);
                        }
                        if (!game_state_json.empty()) {
                            try {
                                nlohmann::json arr = nlohmann::json::parse(game_state_json);
                                Security::Api::game_state_update(token, arr);
                            } catch (...) {}
                        }
                        if (!live_config_json.empty())
                            Security::Api::live_config_update(token, live_config_json);
                    }
                    if (++tick_count >= 30) tick_count = 0;
                }
            }).detach();
        };

        while (!done)
        {
            if (Core::g_Variables.g_Unload) {
                done = true;
                break;
            }
            static bool s_logged_sent = false;
            if (s_cached_game_hwnd == nullptr || (ImGui::GetTime() - s_last_hwnd_check) >= 0.15) {
                s_last_hwnd_check = ImGui::GetTime();

                if (Core::g_AttachedToGame && IsWindow(Core::g_Variables.g_hGameWindow)) {
                    s_cached_game_hwnd = Core::g_Variables.g_hGameWindow;
                } else {
                    HWND fg = GetForegroundWindow();
                    char className[256];
                    if (fg && GetClassNameA(fg, className, sizeof(className)) && strcmp(className, xorstr("grcWindow")) == 0) {
                        s_cached_game_hwnd = fg;
                    } else {
                        s_cached_game_hwnd = Utils::FindFiveMWindow();
                    }
                }
            }
            HWND game_hwnd = s_cached_game_hwnd;

            if (s_hooked_game_hwnd != game_hwnd) {
                if (s_win_event_hook) {
                    UnhookWinEvent(s_win_event_hook);
                    s_win_event_hook = nullptr;
                }
                s_hooked_game_hwnd = game_hwnd;
                if (game_hwnd) {
                    DWORD game_tid = GetWindowThreadProcessId(game_hwnd, nullptr);
                    s_win_event_hook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr, OverlayWinEventProc, 0, game_tid, WINEVENT_OUTOFCONTEXT);
                }
            }

            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            if (var->auth.request_close_overlay && !s_unload_started)
            {
                s_unload_started = true;
                menu_open = false;
                ShowWindow(hwnd, SW_HIDE);
                std::thread([]() {
                    Gui::PerformRestoreAll();
                    Core::g_Variables.g_Unload = true;
                }).detach();
            }

            double t_frame = ImGui::GetTime();
            bool stream_proof = option->param.stream_proof;
            bool dwm_reset = InterlockedCompareExchange(&s_dwm_changed, 0, 1) == 1;
            if (stream_proof != s_last_stream_proof || dwm_reset) {
                s_last_stream_proof = stream_proof;
                BOOL exclude = stream_proof ? TRUE : FALSE;


            }

            static bool s_fse_active = false;
            static double s_last_fse_check = 0.0;
            if (t_frame - s_last_fse_check >= 0.1) {
                s_last_fse_check = t_frame;
                QUERY_USER_NOTIFICATION_STATE quns = QUNS_NOT_PRESENT;
                bool fse_now = SUCCEEDED(SHQueryUserNotificationState(&quns)) && quns == QUNS_RUNNING_D3D_FULL_SCREEN;
                if (fse_now != s_fse_active) {
                    printf("[Overlay] FSE transition: %d -> %d\n", (int)s_fse_active, (int)fse_now);
                    fflush(stdout);
                    s_fse_active = fse_now;
                }
            }
            bool need_ll_hook = s_fse_active && menu_open;
            if (need_ll_hook && !s_mouse_ll_hook) {

                RECT rc = {};
                GetClientRect(hwnd, &rc);
                s_virt_mouse_x  = (float)(rc.right  / 2);
                s_virt_mouse_y  = (float)(rc.bottom / 2);
                s_raw_dx.store(0);
                s_raw_dy.store(0);
                s_last_hw_valid = false;
                ClipCursor(nullptr);
                s_mouse_ll_hook = SetWindowsHookExW(WH_MOUSE_LL, OverlayMouseLLProc, GetModuleHandleW(nullptr), 0);
            } else if (!need_ll_hook && s_mouse_ll_hook) {
                UnhookWindowsHookEx(s_mouse_ll_hook);
                s_mouse_ll_hook = nullptr;
                s_last_hw_valid = false;
            }
            s_ll_eat_mouse.store(need_ll_hook);

            if (menu_open) {
                DisableMouseAccelForMenu();
            } else {
                RestoreMouseAccelIfNeeded();
            }

            if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget();
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (s_ll_eat_mouse.load()) {

                int rdx = s_raw_dx.exchange(0);
                int rdy = s_raw_dy.exchange(0);
                if (rdx != 0 || rdy != 0) {
                    RECT rc = {};
                    GetClientRect(hwnd, &rc);
                    float maxX = (float)(rc.right  > 0 ? rc.right  : 1920);
                    float maxY = (float)(rc.bottom > 0 ? rc.bottom : 1080);
                    s_virt_mouse_x = ImClamp(s_virt_mouse_x + (float)rdx, 0.f, maxX);
                    s_virt_mouse_y = ImClamp(s_virt_mouse_y + (float)rdy, 0.f, maxY);
                }


                ImGui::GetIO().MousePos = ImVec2(s_virt_mouse_x, s_virt_mouse_y);


                ImGui::GetIO().MouseDrawCursor = true;
            } else {
                ImGui::GetIO().MouseDrawCursor = false;
            }

            bool is_auth_phase = (var->auth.current_screen == auth_screen::spinner_pre ||
                                 var->auth.current_screen == auth_screen::login ||
                                 var->auth.current_screen == auth_screen::registering ||
                                 var->auth.current_screen == auth_screen::spinner_post);
            bool is_launch_phase = (var->auth.current_screen == auth_screen::launch);
            bool is_in_game = (var->auth.current_screen == auth_screen::menu);
            if (is_in_game)
                g_MenuInfo.IsOpen = menu_open;

            HWND foreground_window = GetForegroundWindow();
            bool is_game_active = false;
            if (foreground_window == game_hwnd || foreground_window == hwnd) {
                is_game_active = true;
            } else {
                DWORD foreground_pid = 0;
                GetWindowThreadProcessId(foreground_window, &foreground_pid);
                if (foreground_pid != 0 && foreground_pid == Core::g_Variables.ProcIdFiveM) {
                    is_game_active = true;
                }
            }

            if (is_launch_phase && var->auth.launch_clicked)
            {
                var->auth.launch_clicked = false;
                var->auth.launch_state = 1;

                if (!s_attach_thread_running.load() && game_hwnd)
                {
                    s_attach_thread_running.store(true);
                    std::thread([game_hwnd]()
                    {
                        using namespace std::chrono_literals;
                        try {
                            for (int i = 0; i < 50 && !Core::g_AttachedToGame; ++i)
                            {
                                Core::AttachWhenFiveMFound(game_hwnd);
                                if (Core::g_AttachedToGame)
                                    break;
                                std::this_thread::sleep_for(200ms);
                            }
                        } catch (...) {}
                        s_attach_thread_running.store(false);
                    }).detach();
                }
            }
            if (is_launch_phase && var->auth.launch_state == 1)
            {
                if (Core::g_AttachedToGame)
                {
                    var->auth.launch_state = 2;
                    var->auth.launch_fivem_found_time = ImGui::GetTime();
                }
            }
            if (is_launch_phase && var->auth.launch_state == 2)
            {
                double elapsed = ImGui::GetTime() - var->auth.launch_fivem_found_time;
                if (elapsed >= 3.0)
                {
                    var->auth.current_screen = auth_screen::menu;
                    var->auth.launch_state = 0;
                }
            }

            if (is_in_game)
            {
                static bool s_first_in_game = true;
                static bool s_is_reattach = false;
                static bool s_waiting_for_delayed_menu = false;
                static double s_attached_time = 0.0;

                if (game_hwnd && IsWindow(game_hwnd)) {
                    DWORD procId = 0;
                    GetWindowThreadProcessId(game_hwnd, &procId);

                    if (Core::g_AttachedToGame && (procId != Core::g_Variables.ProcIdFiveM || !IsWindow(game_hwnd))) {
                        Core::g_AttachedToGame = false;
                        s_first_in_game = true;
                        s_is_reattach = true;
                        menu_open = false;
                        s_waiting_for_delayed_menu = false;
                    }
                } else if (Core::g_AttachedToGame) {
                    Core::g_AttachedToGame = false;
                    s_first_in_game = true;
                    s_is_reattach = true;
                    menu_open = false;
                    s_waiting_for_delayed_menu = false;
                }

                if (!Core::g_AttachedToGame && game_hwnd && IsWindow(game_hwnd)) {
                    if (!s_attach_thread_running.load()) {
                        s_attach_thread_running.store(true);
                        std::thread([game_hwnd]() {
                            using namespace std::chrono_literals;
                            try {
                                while (!Core::g_AttachedToGame && !Core::g_Variables.g_Unload) {
                                    if (!IsWindow(game_hwnd)) break;
                                    Core::AttachWhenFiveMFound(game_hwnd);
                                    if (Core::g_AttachedToGame) break;
                                    std::this_thread::sleep_for(200ms);
                                }
                            } catch (...) {}
                            s_attach_thread_running.store(false);
                        }).detach();
                    }
                }

                int menuKey = option->param.menu_keybind;
                bool toggle_pressed = (menuKey > 0 && (GetAsyncKeyState(menuKey) & 1) && is_game_active);
                bool trigger_open_logic = false;

                if (toggle_pressed)
                {
                    menu_open = !menu_open;
                    if (menu_open) trigger_open_logic = true;
                }

                if (s_first_in_game && Core::g_AttachedToGame)
                {
                    if (s_is_reattach) {
                        s_first_in_game = false;
                        s_waiting_for_delayed_menu = true;
                        s_attached_time = ImGui::GetTime();
                    } else if (is_game_active) {
                        s_first_in_game = false;
                        menu_open = true;
                        trigger_open_logic = true;
                    }
                }

                if (s_waiting_for_delayed_menu && Core::g_AttachedToGame) {
                    if (ImGui::GetTime() - s_attached_time >= 3.0) {
                        if (is_game_active) {
                            s_waiting_for_delayed_menu = false;
                            menu_open = true;
                            trigger_open_logic = true;
                        }
                    }
                }

                if (trigger_open_logic)
                {
                    bool is_fullscreen = false;
                    if (game_hwnd && IsWindow(game_hwnd)) {
                        RECT game_rect;
                        GetWindowRect(game_hwnd, &game_rect);
                        HMONITOR hMon = MonitorFromWindow(game_hwnd, MONITOR_DEFAULTTONEAREST);
                        MONITORINFO mi = { sizeof(mi) };
                        if (GetMonitorInfoW(hMon, &mi)) {
                            if (game_rect.left == mi.rcMonitor.left && game_rect.top == mi.rcMonitor.top &&
                                game_rect.right == mi.rcMonitor.right && game_rect.bottom == mi.rcMonitor.bottom) {
                                is_fullscreen = true;
                            }
                        }
                    }

                    if (is_fullscreen || s_fse_active)
                    {
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
                    }
                    else
                    {
                        HWND fg = GetForegroundWindow();
                        if (fg && fg != hwnd)
                        {
                            DWORD fgTid = GetWindowThreadProcessId(fg, nullptr);
                            DWORD ourTid = GetCurrentThreadId();
                            if (fgTid != ourTid && AttachThreadInput(fgTid, ourTid, TRUE))
                            {
                                BringWindowToTop(hwnd);
                                ShowWindow(hwnd, SW_SHOW);
                                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                                SetForegroundWindow(hwnd);
                                SetActiveWindow(hwnd);
                                SetFocus(hwnd);
                                AttachThreadInput(fgTid, ourTid, FALSE);
                            }
                            else
                            {
                                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                                SetForegroundWindow(hwnd);
                                SetActiveWindow(hwnd);
                                SetFocus(hwnd);
                            }
                        }
                        else
                        {
                            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                            SetForegroundWindow(hwnd);
                            SetActiveWindow(hwnd);
                            SetFocus(hwnd);
                        }
                    }
                }
                else if (toggle_pressed && !menu_open)
                {
                    if (game_hwnd && IsWindow(game_hwnd)) {
                        SetForegroundWindow(game_hwnd);
                    }
                }
            }

            if (var->auth.authenticated && !var->auth.access_token.empty())
                AntiCrack::SetAccessTokenForReport(var->auth.access_token);

            bool attached_to_fivem = is_in_game || (is_launch_phase && var->auth.launch_state == 2);
            bool sync_with_dashboard = (!is_auth_phase && attached_to_fivem && var->auth.authenticated && !var->auth.access_token.empty());
            if (sync_with_dashboard)
            {
                {
                    std::lock_guard<std::mutex> lock(g_dashboard_mutex);
                    g_dashboard_token = var->auth.access_token;
                }
                start_dashboard_thread();

                if (!s_logged_sent)
                {
                    if (Security::Api::fivem_set_logged(var->auth.access_token, true)) {
                        s_logged_sent = true;
                        g_MenuInfo.IsLogged = true;
                    }
                }

                int validate_result = g_dashboard_validate_result.exchange(-1);
                if (validate_result == 0)
                    option->param.should_unload = true;

                std::vector<Security::Api::FivemCommandItem> commands_local;
                {
                    std::lock_guard<std::mutex> lock(g_dashboard_mutex);
                    commands_local.swap(g_dashboard_pending_commands);
                }
                for (const auto& cmd : commands_local)
                {
                    if (cmd.type == std::string(xorstr("apply_config")) && !cmd.payload.is_null())
                    {
                        int id = cmd.payload.value(xorstr("configId"), 0);
                        if (id <= 0) id = cmd.payload.value(xorstr("config_id"), 0);
                        if (id > 0)
                            var->auth.pending_remote_config_id = id;
                    }
                    else if (cmd.type == std::string(xorstr("apply_live_config")))
                    {
                        if (!cmd.payload.is_null())
                        {
                            try {
                                if (cmd.payload.contains(xorstr("Options")) && cmd.payload[xorstr("Options")].is_object())
                                    OptionsConfig::ApplyOptionsParamFromJson(cmd.payload[xorstr("Options")], &option->param);
                            } catch (...) {}
                        }
                    }
                }

                static double last_live_config_push = 0.0;
                static std::string last_pushed_config_json = "";
                double t = ImGui::GetTime();
                if (last_live_config_push == 0.0) last_live_config_push = t;
                if (t - last_live_config_push >= 5.0)
                {
                    last_live_config_push = t;
                    nlohmann::json payload_obj;
                    payload_obj[xorstr("Options")] = OptionsConfig::OptionsParamToJson(option->param);
                    std::string current_json = payload_obj.dump();

                    if (current_json != last_pushed_config_json) {
                        last_pushed_config_json = current_json;
                        SubmitPendingLiveConfig(current_json);
                    }
                }
            }

            static int lastUnloadKey = 0;
            int unloadKey = option->param.unload_keybind;
            if (unloadKey != lastUnloadKey)
            {
                lastUnloadKey = unloadKey;
                if (unloadKey > 0)
                    GetAsyncKeyState(unloadKey);
            }
            else if ((option->param.should_unload ||
                     (unloadKey > 0 && (GetAsyncKeyState(unloadKey) & 1))) && !s_unload_started)
            {
                s_unload_started = true;
                menu_open = false;
                ShowWindow(hwnd, SW_HIDE);
                std::thread([]() {
                    Gui::PerformRestoreAll();
                    Core::g_Variables.g_Unload = true;
                }).detach();
            }

            if (is_auth_phase || is_launch_phase)
            {
                RECT mon = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
                HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                if (GetMonitorInfoW(hMon, &mi))
                    mon = mi.rcMonitor;
                int mx = mon.left, my = mon.top, mw = mon.right - mon.left, mh = mon.bottom - mon.top;
                SetWindowPos(hwnd, HWND_TOPMOST, mx, my, mw, mh, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
                static bool auth_brought_forward = false;
                if (!auth_brought_forward)
                {
                    SetForegroundWindow(hwnd);
                    auth_brought_forward = true;
                }
            }
            else
            {
                static bool s_last_fse = false;
                bool exstyle_changed = (menu_open != s_last_menu_open) || (is_game_active != s_last_game_active) || (s_fse_active != s_last_fse);
                if (exstyle_changed) {
                    s_last_menu_open = menu_open;
                    s_last_game_active = is_game_active;
                    s_last_fse = s_fse_active;
                }

                static bool s_last_show_state = false;
                static bool s_first_show = true;
                bool should_show = is_game_active || option->param.second_monitor_display;

                if (s_first_show || should_show != s_last_show_state)
                {
                    if (should_show)
                    {
                        ShowWindow(hwnd, SW_SHOW);
                    }
                    else
                    {
                        ShowWindow(hwnd, SW_HIDE);
                    }
                    s_last_show_state = should_show;
                    s_first_show = false;
                }

                if (should_show)
                {
                    if (exstyle_changed) {
                        if (menu_open)
                        {
                            LONG_PTR current_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                            LONG_PTR new_style = s_fse_active
                                ? (WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)
                                : (WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
                            if (current_style != new_style) {
                                SetWindowLongPtr(hwnd, GWL_EXSTYLE, new_style);
                                UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
                                if (s_fse_active) flags |= SWP_NOACTIVATE;
                                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, flags);
                            }
                        }
                        else {
                            LONG_PTR current_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                            LONG_PTR new_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
                            if (current_style != new_style) {
                                SetWindowLongPtr(hwnd, GWL_EXSTYLE, new_style);
                            }
                        }
                    }
                }
                else
                {
                    if (exstyle_changed) {
                        LONG_PTR current_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                        LONG_PTR new_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
                        if (current_style != new_style) {
                            SetWindowLongPtr(hwnd, GWL_EXSTYLE, new_style);
                        }
                    }
                }
            }

            bool force_rect_sync = InterlockedCompareExchange(&s_game_window_moved, 0, 1) == 1;
            if (!is_auth_phase && !is_launch_phase && (force_rect_sync || (t_frame - s_last_rect_sync) >= 0.2))
            {
                s_last_rect_sync = t_frame;
                RECT desired = { 0, 0, 0, 0 };
                RECT game_rect = { 0, 0, 0, 0 };
                bool game_rect_ok = false;
                bool rect_set = false;

                if (game_hwnd && IsWindow(game_hwnd) && !IsIconic(game_hwnd))
                {
                    RECT client_rect;
                    if (GetClientRect(game_hwnd, &client_rect))
                    {
                        POINT top_left = { client_rect.left, client_rect.top };
                        if (ClientToScreen(game_hwnd, &top_left))
                        {
                            game_rect.left = top_left.x;
                            game_rect.top = top_left.y;
                            game_rect.right = game_rect.left + (client_rect.right - client_rect.left);
                            game_rect.bottom = game_rect.top + (client_rect.bottom - client_rect.top);
                            if ((game_rect.right - game_rect.left) > 100 && (game_rect.bottom - game_rect.top) > 100)
                                game_rect_ok = true;
                        }
                    }
                }

                if (option->param.second_monitor_display)
                {
                    static std::vector<MONITORINFO> monitors;
                    static double last_monitor_enum = 0;
                    if (monitors.empty() || (t_frame - last_monitor_enum) > 10.0)
                    {
                        monitors.clear();
                        EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMon, HDC, LPRECT, LPARAM lp) -> BOOL {
                            MONITORINFO mi = { sizeof(mi) };
                            if (GetMonitorInfoW(hMon, &mi))
                                reinterpret_cast<std::vector<MONITORINFO>*>(lp)->push_back(mi);
                            return TRUE;
                        }, reinterpret_cast<LPARAM>(&monitors));

                        std::sort(monitors.begin(), monitors.end(), [](const MONITORINFO& a, const MONITORINFO& b) {
                            if (a.rcMonitor.left != b.rcMonitor.left) return a.rcMonitor.left < b.rcMonitor.left;
                            return a.rcMonitor.top < b.rcMonitor.top;
                        });

                        last_monitor_enum = t_frame;
                    }

                    if (!monitors.empty())
                    {
                        int idx = option->param.display_monitor_index;
                        if (idx < 0 || idx >= (int)monitors.size())
                        {
                            idx = -1;
                            for (int i = 0; i < (int)monitors.size(); ++i) {
                                if (!(monitors[i].dwFlags & MONITORINFOF_PRIMARY)) {
                                    idx = i;
                                    break;
                                }
                            }
                            if (idx == -1) idx = 0;
                        }

                        desired = monitors[idx].rcMonitor;
                        rect_set = true;
                    }
                }

                if (!rect_set)
                {
                    if (game_rect_ok)
                    {
                        desired = game_rect;
                        rect_set = true;
                    }
                    else
                    {
                        HMONITOR hMon = game_hwnd
                            ? MonitorFromWindow(game_hwnd, MONITOR_DEFAULTTONEAREST)
                            : MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
                        MONITORINFO mi = { sizeof(mi) };
                        if (GetMonitorInfoW(hMon, &mi))
                        {
                            desired = mi.rcMonitor;
                            rect_set = true;
                        }
                    }
                }

                    if (rect_set && desired.right > desired.left && desired.bottom > desired.top)
                    {
                        int dw = desired.right - desired.left;
                        int dh = desired.bottom - desired.top;

                        static RECT s_last_desired = { -1, -1, -1, -1 };
                        bool needs_move = false;

                        if (desired.left != s_last_desired.left || desired.top != s_last_desired.top ||
                            desired.right != s_last_desired.right || desired.bottom != s_last_desired.bottom)
                        {
                            needs_move = true;
                            s_last_desired = desired;
                        }

                        if (needs_move) {
                            UINT flags = SWP_NOACTIVATE | SWP_NOSENDCHANGING;
                            SetWindowPos(hwnd, HWND_TOPMOST, desired.left, desired.top, dw, dh, flags);
                        }

                        if (option->param.second_monitor_display) {
                        Core::g_Variables.g_vGameWindowSize = { (float)dw, (float)dh };
                        Core::g_Variables.g_vGameWindowPos = { (float)desired.left, (float)desired.top };
                    } else if (game_rect_ok) {
                        Core::g_Variables.g_vGameWindowSize = { (float)(game_rect.right - game_rect.left), (float)(game_rect.bottom - game_rect.top) };
                        Core::g_Variables.g_vGameWindowPos = { (float)game_rect.left, (float)game_rect.top };
                    } else {
                        Core::g_Variables.g_vGameWindowSize = { (float)dw, (float)dh };
                        Core::g_Variables.g_vGameWindowPos = { (float)desired.left, (float)desired.top };
                    }
                    Core::g_Variables.g_vGameWindowCenter = { Core::g_Variables.g_vGameWindowSize.x / 2.f, Core::g_Variables.g_vGameWindowSize.y / 2.f };
                }
            }

            static double s_last_topmost = 0.0;
            if (!is_auth_phase && !is_launch_phase && (t_frame - s_last_topmost) >= 0.5) {
                s_last_topmost = t_frame;
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
                if (s_last_stream_proof) {
                    BOOL ex = TRUE;


                }
            }

            if ((is_auth_phase || is_launch_phase || (is_in_game && (is_game_active || option->param.second_monitor_display))))
                gui->render();

            if (is_auth_phase || is_launch_phase)
            {
                LONG_PTR current_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                LONG_PTR new_style = WS_EX_LAYERED | WS_EX_TOOLWINDOW;
                if (current_style != new_style) {
                    SetWindowLongPtr(hwnd, GWL_EXSTYLE, new_style);
                }
            }

            if (is_in_game && !menu_open && (is_game_active || option->param.second_monitor_display) && var->auth.authenticated && option->param.watermark) {
                ImGuiIO& wio = ImGui::GetIO();
                int wfps = (int)(wio.Framerate + 0.5f);
                int wms = wfps > 0 ? (int)(1000.0f / (float)wfps + 0.5f) : 0;
                var->watermark.watermark = true;
                var->watermark.content = {xorstr("Rotten"), std::to_string(wfps) + xorstr("FPS"), std::to_string(wms) + xorstr("ms")};
                gui->watermark(xorstr("watermark"), var->watermark.content,
                               watermark_pos::top_right, &var->watermark.watermark);
            }

            if (is_in_game && (is_game_active || option->param.second_monitor_display) && var->auth.authenticated && Core::g_AttachedToGame)
            {
                if (Core::SDK::Pointers::pViewPort) {
                    D3DXMATRIX viewMat = Core::Mem.Read<D3DXMATRIX>(Core::SDK::Pointers::pViewPort + 0x24C);
                    D3DXMatrixTranspose(&viewMat, &viewMat);
                    Core::SDK::Game::SetCachedViewMatrixForFrame(viewMat);
                }
                Core::Features::g_Esp.Draw();
                Core::Features::g_Esp.DrawVehicle();
                Core::Features::g_Esp.DrawObjects();
                Core::Features::g_Esp.DrawRadar();
                ImDrawList* bg = ImGui::GetBackgroundDrawList();
                ImGuiIO& io2 = ImGui::GetIO();
                Core::g_Variables.g_vGameWindowSize = { io2.DisplaySize.x, io2.DisplaySize.y };
                Core::g_Variables.g_vGameWindowCenter = { io2.DisplaySize.x / 2.0f, io2.DisplaySize.y / 2.0f };
                ImVec2 center = ImVec2(io2.DisplaySize.x / 2.0f, io2.DisplaySize.y / 2.0f);

                float defaultFovR = (std::min)(io2.DisplaySize.x, io2.DisplaySize.y) * 0.33f;

                if (option->param.fov_circle)
                {
                    float fovR = option->param.fov_size > 0.0f ? option->param.fov_size : defaultFovR;
                    ImU32 col = ImColor(option->param.fov_color[0], option->param.fov_color[1], option->param.fov_color[2], option->param.fov_color[3]);
                    bg->AddCircle(center, fovR, col, 128, 2.0f);
                }
                if (option->param.silent_fov_circle)
                {
                    ImU32 col2 = ImColor(option->param.silent_fov_color[0], option->param.silent_fov_color[1], option->param.silent_fov_color[2], option->param.silent_fov_color[3]);
                    if (option->param.silent_dual_fov)
                    {
                        if (option->param.silent_fov_near > 0.0f)
                            bg->AddCircle(center, option->param.silent_fov_near, col2, 128, 2.0f);
                        if (option->param.silent_fov_far > 0.0f)
                            bg->AddCircle(center, option->param.silent_fov_far, col2, 128, 2.0f);
                    }
                    else
                    {
                        float fovR = option->param.silent_fov_size > 0.0f ? option->param.silent_fov_size : defaultFovR;
                        bg->AddCircle(center, fovR, col2, 128, 2.0f);
                    }
                }
                if (option->param.triggerbot_fov_circle)
                {
                    float fovR = option->param.triggerbot_fov_size > 0.0f ? option->param.triggerbot_fov_size : defaultFovR;
                    ImU32 col3 = ImColor(option->param.triggerbot_fov_color[0], option->param.triggerbot_fov_color[1], option->param.triggerbot_fov_color[2], option->param.triggerbot_fov_color[3]);
                    bg->AddCircle(center, fovR, col3, 128, 2.0f);
                }
                if (var->friends_tab.enable_setfriend_keybind && var->friends_tab.setfriend_draw_fov && var->friends_tab.setfriend_fov > 0.0f)
                {
                    ImU32 col4 = ImColor(var->friends_tab.setfriend_fov_color[0], var->friends_tab.setfriend_fov_color[1], var->friends_tab.setfriend_fov_color[2], var->friends_tab.setfriend_fov_color[3]);
                    bg->AddCircle(center, var->friends_tab.setfriend_fov, col4, 128, 2.0f);
                }
                if (option->param.teleport_behind_enemy && option->param.teleport_behind_enemy_fov > 0.0f)
                {
                    float tpFov = option->param.teleport_behind_enemy_fov;
                    if (tpFov < 5.0f)
                        tpFov = 20.0f;

                    ImU32 tpCol = ImColor(255, 72, 62, 120);
                    if (var->friends_tab.enable_setfriend_keybind && var->friends_tab.setfriend_draw_fov && var->friends_tab.setfriend_fov > 0.0f) {
                        tpCol = ImColor(var->friends_tab.setfriend_fov_color[0], var->friends_tab.setfriend_fov_color[1], var->friends_tab.setfriend_fov_color[2], var->friends_tab.setfriend_fov_color[3]);
                    }

                    bg->AddCircle(center, tpFov, tpCol, 128, 2.0f);
                }
                if (option->param.teleport_behind_enemy && Core::SDK::Pointers::pLocalPlayer) {
                  CPed* bestPed = Core::SDK::Game::TeleportBehindMarkerTarget;
                  float bestDistSq = 1e30f;
                  float maxRange = option->param.teleport_behind_enemy_range > 0
                    ? option->param.teleport_behind_enemy_range
                    : 1.0f;
                  float maxFov = option->param.teleport_behind_enemy_fov;
                  if (maxFov < 5.0f) maxFov = 20.0f;
                  float fovSq = maxFov * maxFov;

                  if (!bestPed)
                  {
                    std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
                    for ( const auto& entity : Core::SDK::Game::EntityList )
                    {
                      if (!entity.Ped || entity.Ped == Core::SDK::Pointers::pLocalPlayer)
                        continue;
                      if ( option->param.esp_ignore_npcs && !entity.IsPlayer )
                        continue;
                      if ( std::abs( entity.Health ) <= 1.0f )
                        continue;
                      if ( entity.IsFriend )
                        continue;
                      if ( entity.Distance > maxRange )
                        continue;

                      D3DXVECTOR2 pos = Core::SDK::Game::WorldToScreen( entity.Pos );
                      if ( !Core::SDK::Game::IsOnScreen( pos ) )
                        continue;

                      float dx = pos.x - center.x;
                      float dy = pos.y - center.y;
                      float distSq = dx * dx + dy * dy;
                      if ( distSq > fovSq )
                        continue;
                      if ( distSq < bestDistSq ) {
                        bestDistSq = distSq;
                        bestPed = entity.Ped;
                      }
                    }
                  }

                  if ( bestPed ) {
                    Core::SDK::Game::TeleportBehindMarkerTarget = bestPed;
                    D3DXVECTOR3 targetPos = bestPed->GetPos();
                    D3DXVECTOR2 targetScreen = Core::SDK::Game::WorldToScreen( targetPos );
                    bool hasFriendDot = false;
                    if (var->friends_tab.enable_setfriend_keybind && var->friends_tab.setfriend_fov > 0.0f) {
                      if (Core::SDK::Game::IsOnScreen( targetScreen )) {
                        float dotDx = targetScreen.x - center.x;
                        float dotDy = targetScreen.y - center.y;
                        float dotDistSq = dotDx * dotDx + dotDy * dotDy;
                        float friendFovSq = var->friends_tab.setfriend_fov * var->friends_tab.setfriend_fov;
                        hasFriendDot = (dotDistSq <= friendFovSq);
                      }
                    }

                    if (!hasFriendDot) {
                      ImU32 markerCol = ImColor(255, 64, 64, 220);
                      ImU32 markerTextCol = ImColor(255, 255, 255, 220);
                      bg->AddCircleFilled( ImVec2( targetScreen.x, targetScreen.y ), 5.0f, ImColor(0,0,0), 999 );
                      bg->AddCircle( ImVec2( targetScreen.x, targetScreen.y ), 5.0f, markerCol, 999, 1.5f );
                      const char* markerText = xorstr("TP Behind");
                      ImVec2 textSize = ImGui::CalcTextSize( markerText );
                      bg->AddText( ImVec2( targetScreen.x - (textSize.x * 0.5f), targetScreen.y - 22.0f ), ImColor(0,0,0), markerText );
                      bg->AddText( ImVec2( targetScreen.x - (textSize.x * 0.5f), targetScreen.y - 24.0f ), markerTextCol, markerText );
                    }
                  }
                }

                if (option->param.custom_crosshair)
                {
                    ImU32 cr_col = ImColor(option->param.crosshair_color[0], option->param.crosshair_color[1], option->param.crosshair_color[2], option->param.crosshair_color[3]);
                    float s = option->param.crosshair_size;
                    float th = option->param.crosshair_thickness;
                    switch (option->param.crosshair_style)
                    {
                    case 0:
                        bg->AddCircleFilled(center, s, cr_col, 32);
                        break;
                    case 1:
                        bg->AddLine(ImVec2(center.x - s, center.y), ImVec2(center.x + s, center.y), cr_col, th);
                        bg->AddLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y + s), cr_col, th);
                        break;
                    case 2:
                        bg->AddLine(ImVec2(center.x - s, center.y - s), ImVec2(center.x + s, center.y + s), cr_col, th);
                        bg->AddLine(ImVec2(center.x - s, center.y + s), ImVec2(center.x + s, center.y - s), cr_col, th);
                        break;
                    }
                }

                if (var->friends_tab.spectating_ped_id != 0)
                {
                    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(ImGui::GetIO().DisplaySize.x, 30), ImColor(0, 0, 0, 150));
                    ImVec2 TextSize = ImGui::CalcTextSize(xorstr("You are in spectator mode, press ESC to leave"));
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - TextSize.x / 2, 15 - TextSize.y / 2), ImColor(255, 255, 255), xorstr("You are in spectator mode, press ESC to leave"));

                    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
                    {
                        Core::Features::Exploits::SpectatePed(0, false);
                        var->friends_tab.spectating_ped_id = 0;
                    }
                }

                if (option->param.coordinates && Core::SDK::Pointers::pLocalPlayer)
                {
                    D3DXVECTOR3 pos = Core::SDK::Pointers::pLocalPlayer->GetPos();
                    char buf[128];
                    snprintf(buf, sizeof(buf), xorstr("X: %.2f | Y: %.2f | Z: %.2f"), pos.x, pos.y, pos.z);

                    ImVec2 TextSize = ImGui::CalcTextSize(buf);
                    float bar_h = 30.0f;
                    float y_offset = (var->friends_tab.spectating_ped_id != 0) ? 30.0f : 0.0f;

                    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, y_offset), ImVec2(ImGui::GetIO().DisplaySize.x, y_offset + bar_h), ImColor(0, 0, 0, 150));
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - TextSize.x / 2, y_offset + (bar_h / 2) - TextSize.y / 2), ImColor(255, 255, 255), buf);
                }

                Core::SDK::Game::ClearCachedViewMatrixForFrame();
            }

            ImGui::Render();
            const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, option->param.second_monitor_display ? 1.0f : 0.0f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(option->param.vsync ? 1 : 0, 0);

            if (!is_game_active && !menu_open && !option->param.second_monitor_display &&
                !is_auth_phase && !is_launch_phase) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }

            static ULONGLONG s_reveal_tick = 0;
            if (!s_reveal_tick) s_reveal_tick = GetTickCount64();
            if (s_frames_rendered < 5) {
                s_frames_rendered++;
            }
            if (s_frames_rendered >= 5 || GetTickCount64() - s_reveal_tick >= 800) {
                if (s_frames_rendered != 6) {
                    s_frames_rendered = 6;
                    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
                    ShowWindow(hwnd, SW_SHOW);
                    UpdateWindow(hwnd);
                }
                if (GetTickCount64() - s_reveal_tick < 15000) {
                    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
                    MARGINS m = { -1 };
                    DwmExtendFrameIntoClientArea(hwnd, &m);
                }
            }
        }

        g_dashboard_stop = true;

        RestoreMouseAccelIfNeeded();

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        if (s_mouse_ll_hook) { UnhookWindowsHookEx(s_mouse_ll_hook); s_mouse_ll_hook = nullptr; }
        s_ll_overlay_hwnd = nullptr;
        if (s_win_event_hook) { UnhookWinEvent(s_win_event_hook); s_win_event_hook = nullptr; }
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }

}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 2;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CLOSE:
        if (!s_unload_started) {
            s_unload_started = true;
            ShowWindow(hWnd, SW_HIDE);
            std::thread([]() {
                Gui::PerformRestoreAll();
                Core::g_Variables.g_Unload = true;
            }).detach();
        }
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DWMCOMPOSITIONCHANGED: {
        MARGINS m = { -1 };
        DwmExtendFrameIntoClientArea(hWnd, &m);
        SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
        InterlockedExchange(&s_dwm_changed, 1);
        return 0;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
