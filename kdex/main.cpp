#include <Security/Api/api.hpp>
#include <Gui/Overlay/Overlay.hpp>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Core.hpp>
#include <dwmapi.h>
#include <tchar.h>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <Windows.h>

#include "Globals.hpp"
#include <game/Security/LinkerFix.hpp>
#include <game/Security/AntiCrack.hpp>
#include <game/Security/CrashHandler.hpp>
#include <game/Security/UIAccess.hpp>
#include <game/nvidia/nvidia_patch.hpp>

#include <settings/variables.h>

using namespace Core;

bool g_IsInjectedDll = false;
static bool g_IsManualMapped = false;
static HMODULE g_MappedBase = NULL;

typedef void(__cdecl* _PVFV)(void);
typedef int(__cdecl* _PIFV)(void);

static void RunStaticConstructors(HMODULE hModule)
{
}

static void RegisterExceptionTable(HMODULE hModule)
{
	__try {
		auto dosHeader = (PIMAGE_DOS_HEADER)hModule;
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return;

		auto ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
			return;

		auto& exDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
		if (exDir.VirtualAddress == 0 || exDir.Size == 0)
			return;

		auto pTable = (PRUNTIME_FUNCTION)((BYTE*)hModule + exDir.VirtualAddress);
		DWORD count = exDir.Size / sizeof(RUNTIME_FUNCTION);

		RtlAddFunctionTable(pTable, count, (DWORD64)hModule);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

static bool IsModuleInPEB(HMODULE hModule)
{
	HMODULE hTest = NULL;
	GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)hModule,
		&hTest
	);
	return (hTest == hModule);
}

static void ManualMapInit(HMODULE hModule)
{
	g_MappedBase = hModule;
	g_IsManualMapped = !IsModuleInPEB(hModule);

	if (g_IsManualMapped)
    {
		RegisterExceptionTable(hModule);
        RunStaticConstructors(hModule);
    }

	g_hInstance = (HINSTANCE)GetModuleHandleW(NULL);
}

static void SafeCleanupWork()
{
	__try {
		Core::Features::Exploits::RestoreAllVehicles();
		Gui::PerformRestoreAll();

		if (var && var->auth.authenticated && !var->auth.access_token.empty())
			Security::Api::fivem_set_logged(var->auth.access_token, false);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void ClearLoggedState()
{
	Core::g_Variables.g_Unload = true;

	std::thread([]() {
		SafeCleanupWork();
		if (!g_IsInjectedDll)
			LI_FN(TerminateProcess)(LI_FN(GetCurrentProcess)(), 0);
	}).detach();

	g_MenuInfo.IsLogged = false;
	Core::ThreadsStarted = false;
}

HINSTANCE g_hInstance = NULL;
HANDLE g_hMutex = NULL;

static DWORD CheatThreadImpl(LPVOID lpParam);

static void CreateDebugConsole()
{
	if (AllocConsole()) {
		FILE* fDummy;
		freopen_s(&fDummy, xorstr("CONOUT$"), "w", stdout);
		freopen_s(&fDummy, xorstr("CONOUT$"), "w", stderr);
		freopen_s(&fDummy, xorstr("CONIN$"), "r", stdin);
		SetConsoleTitleA(xorstr("kdex-ext Debug Console"));
		SetConsoleOutputCP(CP_UTF8);
		std::ios::sync_with_stdio(true);
	}
}

DWORD WINAPI CheatThread(LPVOID lpParam)
{
	__try {
		return CheatThreadImpl(lpParam);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;
	}
}

static DWORD CheatThreadImpl(LPVOID lpParam) {
    if (g_IsInjectedDll)
        Sleep(0);

	g_hMutex = LI_FN(CreateMutexA)(nullptr, TRUE, xorstr("WindowsHostUpdater"));
	if (LI_FN(GetLastError)() == ERROR_ALREADY_EXISTS)
	{
		if (g_hMutex) { LI_FN(CloseHandle)(g_hMutex); g_hMutex = NULL; }
		return 0;
	}

	if (!g_IsInjectedDll)
		SetProcessDPIAware();

	if (!var) var = std::make_unique<c_variables>();
	if (!gui) gui = std::make_unique<c_gui>();
	if (!option) option = new options();
	Core::g_Config.Initialize();

	CrashHandler::Install();

	if (!g_IsInjectedDll)
		Core::Mem.GetMaxPrivileges(GetCurrentProcess());

	nvidia::runPatch();

	Core::g_Variables.g_hGameWindow = Utils::FindFiveMWindow();
	if (Core::g_Variables.g_hGameWindow) {
		GetWindowThreadProcessId(Core::g_Variables.g_hGameWindow, &Core::g_Variables.ProcIdFiveM);
		if (Core::g_Variables.ProcIdFiveM != 0) {
			auto WindowInfo = Utils::GetWindowPosAndSize(Core::g_Variables.g_hGameWindow);
			Core::g_Variables.g_vGameWindowSize = { (float)WindowInfo.second.x, (float)WindowInfo.second.y };
			Core::g_Variables.g_vGameWindowPos = { (float)WindowInfo.first.x, (float)WindowInfo.first.y };
			Core::g_Variables.g_vGameWindowCenter = { Core::g_Variables.g_vGameWindowSize.x / 2.f, Core::g_Variables.g_vGameWindowSize.y / 2.f };
		}
	}

	if (Core::g_Variables.ProcIdFiveM != 0) {
		std::string procName = Core::Mem.GetNameByPid(Core::g_Variables.ProcIdFiveM);
		if (!procName.empty()) {
			Core::Mem.ProcName = std::wstring(procName.begin(), procName.end());
		}
		Core::Mem.OpenProcByPid();
		Core::StartThreads();
	}

	Gui::cOverlay.Render();

	ClearLoggedState();

	if (g_hMutex) {
		LI_FN(CloseHandle)(g_hMutex);
		g_hMutex = NULL;
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	g_hInstance = hInst;
	if (UIAccess::RelaunchElevated()) return 0;
	if (UIAccess::RelaunchWithUIAccess()) return 0;
	CreateDebugConsole();
	atexit(ClearLoggedState);

	CheatThread(NULL);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_IsInjectedDll = true;
		ManualMapInit(hModule);
		DisableThreadLibraryCalls(hModule);
		if (auto hThread = CreateThread(nullptr, 0, CheatThread, hModule, 0, nullptr))
			CloseHandle(hThread);
		break;
	case DLL_PROCESS_DETACH:
		if (g_IsManualMapped && g_MappedBase)
		{
			__try {
				auto dosHeader = (PIMAGE_DOS_HEADER)g_MappedBase;
				auto ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)g_MappedBase + dosHeader->e_lfanew);
				auto& exDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
				if (exDir.VirtualAddress && exDir.Size) {
					auto pTable = (PRUNTIME_FUNCTION)((BYTE*)g_MappedBase + exDir.VirtualAddress);
					RtlDeleteFunctionTable(pTable);
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {}
		}
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) DWORD WINAPI ManualMapEntry(LPVOID lpParam)
{
	g_IsInjectedDll = true;
	HMODULE hSelf = (HMODULE)lpParam;
	if (!hSelf)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		VirtualQuery(&ManualMapEntry, &mbi, sizeof(mbi));
		hSelf = (HMODULE)mbi.AllocationBase;
	}
	ManualMapInit(hSelf);
	return CheatThread(lpParam);
}
