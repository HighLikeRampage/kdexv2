#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "../Security/xorstr.hpp"

namespace nvidia {
    std::vector<DWORD> getProcessesByName(const std::wstring& processName);
    bool isModuleLoaded(DWORD processID, const std::wstring& moduleName);
    uintptr_t getRemoteModuleBaseAddress(HANDLE hProcess, const wchar_t* moduleName);
    uintptr_t getExportedFunctionAddress(HANDLE hProcess, uintptr_t moduleBase, const wchar_t* moduleName, const char* functionName);
    uintptr_t allocateMemoryNearAddress(HANDLE process, uintptr_t desiredAddress, SIZE_T size, DWORD protection = PAGE_EXECUTE_READWRITE, SIZE_T range = 0x7FFFFFFF);
    bool assembleJumpNearInstruction(uint8_t* buffer, uintptr_t sourceAddress, uintptr_t targetAddress);
    bool writeMemory(HANDLE hProcess, uintptr_t address, const void* buffer, SIZE_T size);
    bool writeMemoryWithProtection(HANDLE hProcess, uintptr_t address, const void* buffer, SIZE_T size);
    bool writeMemoryWithProtectionDynamic(HANDLE hProcess, uintptr_t address, const std::vector<uint8_t>& buffer);

    int patchGetWindowDisplayAffinity(HANDLE hProcess);
    int patchExcludeFromCapture(HANDLE hProcess);
    int patchKernel32Module32FirstW(HANDLE hProcess);
    int patchIsWindowVisible(HANDLE hProcess);
    int patchGetWindowInfo(HANDLE hProcess);
    int patchGetWindowRect(HANDLE hProcess);
    int patchGetWindowLong(HANDLE hProcess);
    int patchEnumChildWindows(HANDLE hProcess);
    int patchEnumWindows(HANDLE hProcess);
    int patchDwmGetWindowAttribute(HANDLE hProcess);
    int patchGetLayeredWindowAttributes(HANDLE hProcess);
    void runPatch();
}
