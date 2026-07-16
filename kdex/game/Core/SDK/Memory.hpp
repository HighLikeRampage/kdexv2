#pragma once
#include <Core/SDK/Structs/Structs.hpp>
#include <Security/xorstr.hpp>
#include <tlhelp32.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <sstream>
#include <cstring>
#include <vector>
#include <regex>
#pragma comment(lib, "ntdll.lib")

typedef struct _MEMORY_REGION {
	DWORD_PTR dwBaseAddr;
	DWORD_PTR dwMemorySize;
}MEMORY_REGION;

namespace Core {

	class MemoryClass {
	public:
		DWORD ProcId;
		HANDLE ProcHandle;
		std::wstring ProcName;
		uintptr_t ModBase, ModBaseSize;

		DWORD GetPidByName(const char* ProcName);
		std::string GetNameByPid(DWORD Pid);
		uintptr_t GetModuleBaseAddr(DWORD ProcId, const char* ModuleName, uintptr_t* ModSize);

		uint64_t GetModuleBaseByName(DWORD Pid, std::wstring ModuleName);

		uint64_t GetModuleBaseSizeByName(DWORD Pid, std::wstring ModuleName);

		BOOL OpenProcByPid();
		BOOL OpenProcByPid(DWORD pid);
		BOOL GetMaxPrivileges(HANDLE hProc);

		std::string ReadString(uintptr_t Addr);
		std::vector<uint8_t> ReadBytes(uintptr_t Addr, size_t Size);
		BOOL WriteBytes(uintptr_t Addr, std::vector<uint8_t> Bytes);
		bool WriteProcessMemoryImpl(uint64_t WriteAddress, LPVOID Value, SIZE_T Size);
		BOOL PatchFunc(uintptr_t Addr, int NopCount);

		uintptr_t FindSignature(std::vector<uint8_t> Signature, uintptr_t ModuleBase = 0, uintptr_t ModuleBaseSize = 0);
		uintptr_t FindSignatureStr(std::string Pattern, uintptr_t ModuleBase = 0, uintptr_t ModuleBaseSize = 0);
		uintptr_t PatternScan(std::vector<uint8_t> Pattern, int InstructionLength);
		uintptr_t FindSignatureBypass(std::vector<uint8_t> Signature, uintptr_t ModuleBase, uintptr_t ModuleBaseSize);

		template <typename T>
		T Read(uintptr_t Addr) {
			T value = { };
			ZwReadVirtualMemory(ProcHandle, (LPCVOID)Addr, &value, sizeof(value), NULL);
			return value;
		}

		template <typename T>
		BOOL Write(uintptr_t Addr, T Value) {
			return ZwWriteVirtualMemory(ProcHandle, (LPVOID)Addr, &Value, sizeof(Value), NULL);
		}

		template <typename T>
		BOOL WriteProtected(uintptr_t Addr, T Value) {
			if (!ProcHandle || !Addr)
				return FALSE;

			DWORD oldProtect = 0;
			if (!VirtualProtectEx(ProcHandle, reinterpret_cast<LPVOID>(Addr), sizeof(Value), PAGE_EXECUTE_READWRITE, &oldProtect))
				return FALSE;

			BOOL ok = WriteProcessMemory(ProcHandle, reinterpret_cast<LPVOID>(Addr), &Value, sizeof(Value), nullptr) != FALSE;
			DWORD tempProtect = 0;
			VirtualProtectEx(ProcHandle, reinterpret_cast<LPVOID>(Addr), sizeof(Value), oldProtect, &tempProtect);
			return ok;
		}

		BOOL ReadRaw(uintptr_t Addr, void* Buffer, size_t Size) {
			return ZwReadVirtualMemory(ProcHandle, (LPCVOID)Addr, Buffer, Size, NULL) == 0;
		}

		BOOL WriteRaw(uintptr_t Addr, const void* Buffer, size_t Size) {
			return ZwWriteVirtualMemory(ProcHandle, (LPVOID)Addr, (LPVOID)Buffer, Size, NULL) == 0;
		}

		uintptr_t CreateCodeCave(size_t Size) {
			LPVOID CaveAddress = VirtualAllocEx(ProcHandle, nullptr, Size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (!CaveAddress) {
				CaveAddress = VirtualAllocEx(ProcHandle, nullptr, Size, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (CaveAddress) {
					if (!VirtualAllocEx(ProcHandle, CaveAddress, Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
						VirtualFreeEx(ProcHandle, CaveAddress, 0, MEM_RELEASE);
						CaveAddress = nullptr;
					}
				}
			}
			return (uintptr_t)CaveAddress;
		}

		BOOL FreeCave(uintptr_t Addr) {
			return VirtualFreeEx(ProcHandle, (LPVOID)Addr, 0, MEM_RELEASE);
		}

		BOOL HookJMP(uintptr_t HookAddress, uintptr_t JmpToAddress) {
			static const BYTE jmpPatch[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
			WriteRaw(HookAddress, jmpPatch, sizeof(jmpPatch));
			Write<uintptr_t>(HookAddress + 6, JmpToAddress);
			return true;
		}

		uintptr_t ResolveRelativeAddress(uintptr_t Addr, int InstructionLength)
		{
			int32_t Offset = Read<int32_t>(Addr + (InstructionLength - 4));
			return Addr + InstructionLength + Offset;
		}

		template <typename T>
		T ReadHandle(HANDLE hProcess, uintptr_t Addr) {
			T value{};
			if (!hProcess || !Addr) return value;
			ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(Addr), &value, sizeof(T), nullptr);
			return value;
		}

		template <typename T>
		BOOL WriteHandle(HANDLE hProcess, uintptr_t Addr, T Value) {
			if (!hProcess || !Addr) return FALSE;
			return WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(Addr), &Value, sizeof(T), nullptr) != FALSE;
		}

		BOOL WriteBytesHandle(HANDLE hProcess, uintptr_t Addr, const std::vector<uint8_t>& Bytes) {
			if (!hProcess || !Addr || Bytes.empty()) return FALSE;
			return WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(Addr), Bytes.data(), Bytes.size(), nullptr) != FALSE;
		}

		std::vector<uint8_t> Pattern2Vector(std::string Pattern) {
			std::vector<uint8_t> result;
			std::stringstream ss(Pattern);
			std::string token;

			while (ss >> token) {
				if (token == xorstr("??")) {
					result.push_back(0x00);
				}
				else if (token == xorstr("?")) {
					result.push_back(0x00);
				}
				else {
					uint8_t value = std::stoul(token, nullptr, 16);
					result.push_back(value);
				}
			}

			return result;
		}

	};

	extern MemoryClass Mem;
}
