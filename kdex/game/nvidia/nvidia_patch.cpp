#include "nvidia_patch.hpp"
#include <tlhelp32.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdio>

namespace nvidia {

    static std::wstring toLower(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }

    std::vector<DWORD> getProcessesByName(const std::wstring& processName) {
        std::vector<DWORD> processIDs;
        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return processIDs;
        }

        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            return processIDs;
        }

        std::wstring targetName = toLower(processName);
        do {
            if (toLower(pe32.szExeFile) == targetName) {
                processIDs.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hProcessSnap, &pe32));

        CloseHandle(hProcessSnap);
        return processIDs;
    }

    bool isModuleLoaded(DWORD processID, const std::wstring& moduleName) {
        HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
        MODULEENTRY32 me32;
        me32.dwSize = sizeof(MODULEENTRY32);

        hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
        if (hModuleSnap == INVALID_HANDLE_VALUE) {
            return false;
        }

        if (!Module32First(hModuleSnap, &me32)) {
            CloseHandle(hModuleSnap);
            return false;
        }

        std::wstring targetModuleName = toLower(moduleName);

        do {
            if (toLower(me32.szModule) == targetModuleName || toLower(me32.szExePath) == targetModuleName) {
                CloseHandle(hModuleSnap);
                return true;
            }
        } while (Module32Next(hModuleSnap, &me32));

        CloseHandle(hModuleSnap);
        return false;
    }

    uintptr_t getRemoteModuleBaseAddress(HANDLE hProcess, const wchar_t* moduleName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hProcess));
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32 me;
        me.dwSize = sizeof(MODULEENTRY32);
        uintptr_t moduleBase = 0;
        if (Module32First(hSnapshot, &me)) {
            do {
                if (_wcsicmp(me.szModule, moduleName) == 0) {
                    moduleBase = (uintptr_t)me.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &me));
        }
        CloseHandle(hSnapshot);
        return moduleBase;
    }

    uintptr_t getExportedFunctionAddress(HANDLE hProcess, uintptr_t moduleBase, const wchar_t* moduleName, const char* functionName) {
        HMODULE hLocalModule = LoadLibraryW(moduleName);
        if (!hLocalModule) return 0;

        FARPROC localProcAddress = GetProcAddress(hLocalModule, functionName);
        if (!localProcAddress) {
            FreeLibrary(hLocalModule);
            return 0;
        }

        uintptr_t offset = (uintptr_t)localProcAddress - (uintptr_t)hLocalModule;
        FreeLibrary(hLocalModule);
        return moduleBase + offset;
    }

    uintptr_t allocateMemoryNearAddress(HANDLE process, uintptr_t desiredAddress, SIZE_T size, DWORD protection, SIZE_T range) {
        const SIZE_T step = 0x1000;
        uintptr_t baseAddress = desiredAddress - range;
        uintptr_t endAddress = desiredAddress + range;

        for (uintptr_t address = baseAddress; address < endAddress; address += step) {
            void* allocatedMemory = VirtualAllocEx(
                process,
                reinterpret_cast<void*>(address),
                size,
                MEM_RESERVE | MEM_COMMIT,
                protection
            );

            if (allocatedMemory != NULL) {
                return reinterpret_cast<uintptr_t>(allocatedMemory);
            }
        }
        return NULL;
    }

    bool assembleJumpNearInstruction(uint8_t* buffer, uintptr_t sourceAddress, uintptr_t targetAddress) {
        intptr_t jumpOffset = targetAddress - (sourceAddress + 5);
        if (std::abs(jumpOffset) > 0x7FFFFFFF) {
            return false;
        }
        buffer[0] = 0xE9;
        *reinterpret_cast<int32_t*>(buffer + 1) = static_cast<int32_t>(jumpOffset);
        return true;
    }

    bool writeMemory(HANDLE hProcess, uintptr_t address, const void* buffer, SIZE_T size) {
        SIZE_T written;
        return WriteProcessMemory(hProcess, reinterpret_cast<void*>(address), buffer, size, &written) && written == size;
    }

    bool writeMemoryWithProtection(HANDLE hProcess, uintptr_t address, const void* buffer, SIZE_T size) {
        DWORD oldProtect;
        if (!VirtualProtectEx(hProcess, reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }
        bool success = writeMemory(hProcess, address, buffer, size);
        VirtualProtectEx(hProcess, reinterpret_cast<void*>(address), size, oldProtect, &oldProtect);
        return success;
    }

    bool writeMemoryWithProtectionDynamic(HANDLE hProcess, uintptr_t address, const std::vector<uint8_t>& buffer) {
        return writeMemoryWithProtection(hProcess, address, buffer.data(), buffer.size());
    }

    int patchGetWindowDisplayAffinity(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("GetWindowDisplayAffinity"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0xC7, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchEnumChildWindows(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("EnumChildWindows"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchGetWindowRect(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("GetWindowRect"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0x31, 0xC0, 0x48, 0x89, 0x02, 0x48, 0x89, 0x42, 0x08, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchGetWindowLong(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        const char* functions[] = { xorstr("GetWindowLongA"), xorstr("GetWindowLongPtrA"), xorstr("GetWindowLongW"), xorstr("GetWindowLongPtrW") };

        for (const char* funcName : functions) {
            uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), funcName);
            if (!remoteTargetAddress) continue;

            uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
            if (!allocatedMemory) continue;

            if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x83, 0xFA, 0xF8, 0x74, 0x0C, 0x83, 0xFA, 0xF0, 0x74, 0x07, 0x48, 0x31, 0xC0, 0xC3, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3, 0xB8, 0x80, 0x00, 0x00, 0x00, 0xC3 })) {
                continue;
            }

            uint8_t jmpInstructionBytes[5];
            if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
                continue;
            }

            uint8_t buffer[6];
            memcpy(buffer, jmpInstructionBytes, 5);
            buffer[5] = 0x90;

            writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6);
        }
        return 0;
    }

    int patchKernel32Module32FirstW(HANDLE hProcess) {
        uintptr_t moduleBaseAddress = getRemoteModuleBaseAddress(hProcess, xorstr(L"KERNEL32.DLL"));
        if (!moduleBaseAddress) return 1;

        uintptr_t functionAddress = getExportedFunctionAddress(hProcess, moduleBaseAddress, xorstr(L"KERNEL32.DLL"), xorstr("Module32FirstW"));
        if (!functionAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, functionAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0x31, 0xC0, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, functionAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[7];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;
        buffer[6] = 0x90;

        if (!writeMemoryWithProtection(hProcess, functionAddress, buffer, 7)) {
            return 1;
        }
        return 0;
    }

    int patchExcludeFromCapture(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("IsWindowExcludedFromCapture"));
        if (!remoteTargetAddress) {
            return patchGetWindowDisplayAffinity(hProcess);
        }

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchIsWindowVisible(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("IsWindowVisible"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0x31, 0xC0, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchGetWindowInfo(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("GetWindowInfo"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0xC7, 0x42, 0x24, 0x00, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchDwmGetWindowAttribute(HANDLE hProcess) {
        uintptr_t remoteDwmApiBase = getRemoteModuleBaseAddress(hProcess, xorstr(L"dwmapi.dll"));
        if (!remoteDwmApiBase) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteDwmApiBase, xorstr(L"dwmapi.dll"), xorstr("DwmGetWindowAttribute"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x83, 0xFA, 0x0E, 0x75, 0x07, 0xC7, 0x01, 0x01, 0x00, 0x00, 0x00, 0x31, 0xC0, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchGetLayeredWindowAttributes(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("GetLayeredWindowAttributes"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0x85, 0xD2, 0x74, 0x03, 0xC6, 0x02, 0x00, 0x48, 0x85, 0xC9, 0x74, 0x03, 0xC6, 0x01, 0x00, 0x4D, 0x85, 0xC0, 0x74, 0x04, 0x41, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    int patchEnumWindows(HANDLE hProcess) {
        uintptr_t remoteUser32Base = getRemoteModuleBaseAddress(hProcess, xorstr(L"USER32.dll"));
        if (!remoteUser32Base) return 1;

        uintptr_t remoteTargetAddress = getExportedFunctionAddress(hProcess, remoteUser32Base, xorstr(L"USER32.dll"), xorstr("EnumWindows"));
        if (!remoteTargetAddress) return 1;

        uintptr_t allocatedMemory = allocateMemoryNearAddress(hProcess, remoteTargetAddress, 0x1000);
        if (!allocatedMemory) return 1;

        if (!writeMemoryWithProtectionDynamic(hProcess, allocatedMemory, { 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0xC3 })) {
            return 1;
        }

        uint8_t jmpInstructionBytes[5];
        if (!assembleJumpNearInstruction(jmpInstructionBytes, remoteTargetAddress, allocatedMemory)) {
            return 1;
        }

        uint8_t buffer[6];
        memcpy(buffer, jmpInstructionBytes, 5);
        buffer[5] = 0x90;

        if (!writeMemoryWithProtection(hProcess, remoteTargetAddress, buffer, 6)) {
            return 1;
        }
        return 0;
    }

    void runPatch() {
        std::vector<std::wstring> targetProcesses = {
            xorstr(L"nvcontainer.exe"),
            xorstr(L"NVIDIA Share.exe"),
            xorstr(L"nvsphelper64.exe"),
            xorstr(L"NVIDIA Overlay.exe"),
            xorstr(L"NVIDIA app.exe"),
            xorstr(L"NVIDIA Web Helper.exe"),
            xorstr(L"nvsphelper.exe"),
            xorstr(L"nvidia-share.exe"),
        };

        printf("[nvidia] runPatch enter\n"); fflush(stdout);
        int total_procs = 0;
        int total_patched = 0;
        int total_openfail = 0;
        for (const auto& procName : targetProcesses) {
            std::vector<DWORD> processIDs = getProcessesByName(procName);
            printf("[nvidia] scanning %ls: %zu instances\n", procName.c_str(), processIDs.size()); fflush(stdout);
            for (DWORD processID : processIDs) {
                total_procs++;
                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
                if (!hProcess) {
                    total_openfail++;
                    printf("[nvidia]   pid=%lu OpenProcess FAILED err=%lu\n", processID, GetLastError()); fflush(stdout);
                    continue;
                }
                int r1 = patchGetWindowDisplayAffinity(hProcess);
                int r2 = patchExcludeFromCapture(hProcess);
                int r3 = patchKernel32Module32FirstW(hProcess);
                int r4 = patchIsWindowVisible(hProcess);
                int r5 = patchGetWindowInfo(hProcess);
                int r6 = patchGetWindowRect(hProcess);
                int r7 = patchGetWindowLong(hProcess);
                int r8 = patchEnumChildWindows(hProcess);
                int r9 = patchEnumWindows(hProcess);
                int r10 = patchDwmGetWindowAttribute(hProcess);
                int r11 = patchGetLayeredWindowAttributes(hProcess);
                CloseHandle(hProcess);
                total_patched++;
                printf("[nvidia]   pid=%lu patched: WDA=%d EFC=%d M32=%d IWV=%d GWI=%d GWR=%d GWL=%d ECW=%d EW=%d DGWA=%d GLWA=%d\n",
                    processID, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11);
                fflush(stdout);
            }
        }
        printf("[nvidia] runPatch done: procs_found=%d patched=%d open_failed=%d\n",
            total_procs, total_patched, total_openfail); fflush(stdout);
    }
}
