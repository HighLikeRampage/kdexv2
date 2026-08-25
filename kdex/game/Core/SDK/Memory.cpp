#include "Memory.hpp"
#include <Core/Variables.hpp>
#include <filesystem>
#include <Auth/lazyimporter.hpp>
#include <Globals.hpp>
#include "../../Security/xorstr.hpp"

HANDLE AttachedProcessHandle;
DWORD AttachedProcessPid;

namespace Core {

	VariablesClass g_Variables;
	MemoryClass Mem;

	uintptr_t MemoryClass::PatternScan(std::vector<uint8_t> Pattern, int InstructionLength)
	{
		uintptr_t Address = FindSignature(Pattern);

		if (InstructionLength != 0)
		{
			Address = ResolveRelativeAddress(Address, InstructionLength);
		}

		return Address;
	}

	uintptr_t MemoryClass::FindSignature(std::vector<uint8_t> Signature, uintptr_t ModuleBase, uintptr_t ModuleBaseSize)
	{
		const size_t blockSize = (4096 * 4);
		std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(blockSize);

		DWORD oldProtect;
		size_t signatureSize = Signature.size();

		uintptr_t ModulBase;
		uintptr_t ModulBaseSize;

		if (ModuleBase != 0 && ModuleBaseSize != 0) {
			ModulBase = ModuleBase;
			ModulBaseSize = ModuleBaseSize;
		}
		else {
			ModulBase = ModBase;
			ModulBaseSize = ModBaseSize;
		}

		for (uintptr_t address = ModulBase; address < ModulBase + ModulBaseSize; address += blockSize)
		{
			if (!VirtualProtectEx(ProcHandle, (LPVOID)address, blockSize, PAGE_EXECUTE_READWRITE, &oldProtect)) { continue; }

			SIZE_T bytesRead;
			if (!ReadProcessMemory(ProcHandle, (void*)address, data.get(), blockSize, &bytesRead)) {
				VirtualProtectEx(ProcHandle, (LPVOID)address, blockSize, oldProtect, NULL);
				continue;
			}

			VirtualProtectEx(ProcHandle, (LPVOID)address, blockSize, oldProtect, NULL);

			for (uintptr_t i = 0; i < bytesRead; i++)
			{
				for (uintptr_t j = 0; j < signatureSize; j++)
				{
					if (Signature[j] == 0x00)
						continue;

					if (data[i + j] != Signature[j])
						break;

					if (j == signatureSize - 1)
						return (address + i);
				}
			}
		}

		return 0x0;
	}

	uintptr_t MemoryClass::FindSignatureBypass(std::vector<uint8_t> Signature, uintptr_t ModuleBase, uintptr_t ModuleBaseSize)
	{
		const size_t blockSize = (4096 * 4);
		std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(blockSize);

		DWORD oldProtect;
		size_t signatureSize = Signature.size();

		uintptr_t ModulBase;
		uintptr_t ModulBaseSize;

		if (ModuleBase != 0 && ModuleBaseSize != 0) {
			ModulBase = ModuleBase;
			ModulBaseSize = ModuleBaseSize;
		}
		else {
			ModulBase = ModBase;
			ModulBaseSize = ModBaseSize;
		}

		for (uintptr_t address = ModulBase; address < ModulBase + ModulBaseSize; address += blockSize)
		{
			SIZE_T bytesRead;
			if (!ReadProcessMemory(ProcHandle, (void*)address, data.get(), blockSize, &bytesRead)) {
				continue;
			}

			for (uintptr_t i = 0; i < bytesRead; i++)
			{
				for (uintptr_t j = 0; j < signatureSize; j++)
				{
					if (Signature[j] == 0x00)
						continue;

					if (data[i + j] != Signature[j])
						break;

					if (j == signatureSize - 1)
						return (address + i);
				}
			}
		}

		return 0x0;
	}

	uintptr_t MemoryClass::FindSignatureStr(std::string Pattern, uintptr_t ModuleBase, uintptr_t ModuleBaseSize) {
		std::vector<uint8_t> signature = Pattern2Vector(Pattern);
		if (ModuleBase != 0 && ModuleBaseSize != 0) {
			return FindSignature(signature, ModuleBase, ModuleBaseSize);
		}
		else {
			return FindSignature(signature);
		}
	}

	std::vector<uintptr_t> MemoryClass::FindAllPatterns(std::vector<int> Pattern, uintptr_t ModuleBase, uintptr_t ModuleBaseSize)
	{
		std::vector<uintptr_t> results;
		if (Pattern.empty())
			return results;

		const size_t blockSize = (4096 * 4);
		std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(blockSize);

		size_t signatureSize = Pattern.size();

		uintptr_t ModulBase;
		uintptr_t ModulBaseSize;

		if (ModuleBase != 0 && ModuleBaseSize != 0) {
			ModulBase = ModuleBase;
			ModulBaseSize = ModuleBaseSize;
		}
		else {
			ModulBase = ModBase;
			ModulBaseSize = ModBaseSize;
		}

		for (uintptr_t address = ModulBase; address < ModulBase + ModulBaseSize; address += blockSize - signatureSize + 1)
		{
			SIZE_T bytesRead;
			size_t readSize = blockSize;
			if (address + readSize > ModulBase + ModulBaseSize)
				readSize = (ModulBase + ModulBaseSize) - address;

			if (!ReadProcessMemory(ProcHandle, (void*)address, data.get(), readSize, &bytesRead)) {
				continue;
			}

			if (bytesRead < signatureSize)
				continue;

			for (uintptr_t i = 0; i + signatureSize <= bytesRead; i++)
			{
				bool match = true;
				for (uintptr_t j = 0; j < signatureSize; j++)
				{
					if (Pattern[j] < 0)
						continue;
					if (data[i + j] != (uint8_t)Pattern[j])
					{
						match = false;
						break;
					}
				}
				if (match)
				{
					results.push_back(address + i);
				}
			}
		}

		return results;
	}

	std::vector<uint8_t> MemoryClass::ReadBytes(uintptr_t Addr, size_t Size) {
		std::vector<uint8_t> bytes(Size);
		size_t bytesRead = 0;
		ZwReadVirtualMemory(ProcHandle, (LPCVOID)Addr, bytes.data(), Size, &bytesRead);
		bytes.resize(bytesRead);
		return bytes;
	}

	BOOL MemoryClass::WriteBytes(uintptr_t Addr, std::vector<uint8_t> Bytes) {
		NTSTATUS status = ZwWriteVirtualMemory(ProcHandle, (LPVOID)Addr, Bytes.data(), Bytes.size(), NULL);
		return NT_SUCCESS(status);
	}

	bool MemoryClass::WriteProcessMemoryImpl(uint64_t WriteAddress, LPVOID Value, SIZE_T Size)
	{
		if (AttachedProcessHandle && AttachedProcessPid)
		{
			if (WriteProcessMemory(AttachedProcessHandle, (LPVOID)WriteAddress, Value, Size, NULL))
			{
				return true;
			}
		}

		return false;
	}

	BOOL MemoryClass::PatchFunc(uintptr_t Addr, int NopCount)
	{
		if (!NopCount || NopCount > 64) return FALSE;
		uint8_t nops[64];
		memset(nops, 0x90, static_cast<size_t>(NopCount));
		return NT_SUCCESS(ZwWriteVirtualMemory(ProcHandle, (LPVOID)Addr, nops, static_cast<SIZE_T>(NopCount), NULL));
	}

	BOOL MemoryClass::GetMaxPrivileges(HANDLE hProc) {
		HANDLE h_Token;
		DWORD dw_TokenLength;
		if (OpenProcessToken(hProc, TOKEN_READ | TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &h_Token))
		{
			BYTE privBuf[sizeof(TOKEN_PRIVILEGES) * 100];
			TOKEN_PRIVILEGES* privilages = reinterpret_cast<TOKEN_PRIVILEGES*>(privBuf);
			if (GetTokenInformation(h_Token, TokenPrivileges, privilages, sizeof(TOKEN_PRIVILEGES) * 100, &dw_TokenLength))
			{
				for (int i = 0; i < (int)privilages->PrivilegeCount; i++)
				{
					privilages->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
				}
				if (AdjustTokenPrivileges(h_Token, false, privilages, sizeof(TOKEN_PRIVILEGES) * 100, NULL, NULL))
				{
					return true;
				}
			}
		}

		return false;
	}

	std::string MemoryClass::ReadString(uintptr_t Addr) {
		char buffer[256];
		SIZE_T bytesRead = 0;
		ZwReadVirtualMemory(ProcHandle, (LPVOID)Addr, buffer, sizeof(buffer), &bytesRead);
		if (!bytesRead) return "";
		if (bytesRead == sizeof(buffer)) {
			size_t len = strnlen(buffer, sizeof(buffer));
			if (len == sizeof(buffer)) return "";
			return std::string(buffer, len);
		}
		buffer[bytesRead] = '\0';
		return std::string(buffer);
	}

	DWORD MemoryClass::GetPidByName(const char* ProcName) {
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			return 0;
		}

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (!Process32First(hSnapshot, &pe32)) {
			CloseHandle(hSnapshot);
			return 0;
		}

		std::string procNameStr(ProcName);
		std::wstring procNameW(procNameStr.begin(), procNameStr.end());

		DWORD pid = 0;
		do {
			if (_wcsicmp(pe32.szExeFile, procNameW.c_str()) == 0) {
				pid = pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));

		CloseHandle(hSnapshot);
		return pid;
	}

	std::string MemoryClass::GetNameByPid(DWORD Pid) {
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Pid);
		if (!hProcess) { return ""; }

		WCHAR processName[MAX_PATH];
		if (!GetModuleFileNameExW(hProcess, NULL, processName, MAX_PATH)) { return ""; }

		CloseHandle(hProcess);

		std::wstring ws(processName);
		std::string fullPath(ws.begin(), ws.end());
		size_t pos = fullPath.find_last_of(xorstr("\\/"));
		if (pos != std::string::npos) {
			return fullPath.substr(pos + 1);
		}
		else {
			return fullPath;
		}
	}

	uintptr_t MemoryClass::GetModuleBaseAddr(DWORD ProcId, const char* ModuleName, uintptr_t* ModSize) {
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcId);

		if (hSnapShot == INVALID_HANDLE_VALUE) { return 0; }

		MODULEENTRY32 ModuleEntry;
		ModuleEntry.dwSize = sizeof(MODULEENTRY32);

		std::string modNameStr(ModuleName);
		std::wstring modNameW(modNameStr.begin(), modNameStr.end());

		if (Module32First(hSnapShot, &ModuleEntry)) {
			do {
				if (_wcsicmp(ModuleEntry.szModule, modNameW.c_str()) == 0) {
					CloseHandle(hSnapShot);
					if (ModSize) *ModSize = (uintptr_t)ModuleEntry.modBaseSize;
					return (uintptr_t)ModuleEntry.modBaseAddr;
				}
			} while (Module32Next(hSnapShot, &ModuleEntry));
		}
		CloseHandle(hSnapShot);
		if (ModSize) *ModSize = 0;
		return 0;
	}

	BOOL MemoryClass::OpenProcByPid() {
		if (g_Variables.ProcIdFiveM == 0) {
			return false;
		}

		if (ProcHandle) {
			CloseHandle(ProcHandle);
			ProcHandle = nullptr;
		}

		ProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, g_Variables.ProcIdFiveM);
		if (!ProcHandle) {
			return false;
		};

		ProcId = g_Variables.ProcIdFiveM;

		std::string procNameStr(Mem.ProcName.begin(), Mem.ProcName.end());
		Mem.ModBase = Mem.GetModuleBaseAddr(g_Variables.ProcIdFiveM, procNameStr.c_str(), &ModBaseSize);
		if (Mem.ModBase == 0) {
			return false;
		}

		return true;
	}

	BOOL MemoryClass::OpenProcByPid(DWORD pid) {
		if (ProcHandle) {
			CloseHandle(ProcHandle);
			ProcHandle = nullptr;
		}

		ProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (!ProcHandle) {
			return false;
		}

		ProcId = pid;

		HMODULE hMods[1024];
		DWORD cbNeeded;
		if (EnumProcessModules(ProcHandle, hMods, sizeof(hMods), &cbNeeded)) {
			MODULEINFO mi;
			if (GetModuleInformation(ProcHandle, hMods[0], &mi, sizeof(mi))) {
				ModBase = (uintptr_t)mi.lpBaseOfDll;
				ModBaseSize = (uintptr_t)mi.SizeOfImage;
			}
		}

		return true;
	}

}
