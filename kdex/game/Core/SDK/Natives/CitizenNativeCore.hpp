#pragma once
#include <Core/SDK/Memory.hpp>
#include <Security/xorstr.hpp>

#include <Windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cwchar>
#include <vector>

namespace CitizenNativeCore {

	struct SCitizenCore {
		bool      valid       = false;
		uintptr_t coreBase    = 0;
		uintptr_t coreSize    = 0;
		uintptr_t nativeTable = 0;
		size_t    entryCount  = 0;
	};

	struct FallbackOffsets {
		uintptr_t table;
	};

	inline const std::vector<FallbackOffsets>& KnownTableOffsets() {
		static const std::vector<FallbackOffsets> k = {
			{ 0x10BD58 }, { 0x10BCE8 }, { 0x10BC78 },
			{ 0x10C2A8 }, { 0x10C3A8 }, { 0x10C4A8 },
			{ 0x10C5A8 }, { 0x10C6A8 }, { 0x10C7A8 },
		};
		return k;
	}

	inline bool IsPlausibleNativeEntry(HANDLE hProc, uintptr_t ent) {
		if (!hProc || ent < 0x10000ULL) return false;
		uint64_t handler = 0;
		SIZE_T got = 0;
		if (!ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(ent + 0x18), &handler, sizeof(handler), &got) || got != sizeof(handler))
			return false;
		return handler > 0x10000ULL;
	}

	inline uintptr_t FindCoreModule(DWORD pid, uintptr_t& outSize) {
		outSize = 0;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (snap == INVALID_HANDLE_VALUE) return 0;

		MODULEENTRY32W me{ sizeof(me) };
		uintptr_t base = 0;
		if (Module32FirstW(snap, &me)) {
			do {
				if (_wcsnicmp(me.szModule, L"citizen-scripting-core", 22) == 0) {
					base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
					outSize = me.modBaseSize;
					break;
				}
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
		return base;
	}

	inline SCitizenCore Resolve(HANDLE hProc, DWORD pid) {
		SCitizenCore out{};
		if (!hProc || !pid) return out;

		uintptr_t coreSize = 0;
		const uintptr_t coreBase = FindCoreModule(pid, coreSize);
		if (!coreBase || !coreSize) return out;

		out.coreBase = coreBase;
		out.coreSize = coreSize;

		auto readU64 = [hProc](uintptr_t addr) -> uint64_t {
			uint64_t v = 0; SIZE_T got = 0;
			if (!ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(addr), &v, sizeof(v), &got) || got != sizeof(v))
				return 0;
			return v;
		};

		for (const auto& fb : KnownTableOffsets()) {
			const uintptr_t tableVA = coreBase + fb.table;
			const uint64_t tBegin = readU64(tableVA);
			const uint64_t tEnd   = readU64(tableVA + 8);
			if (!tBegin || !tEnd || tEnd <= tBegin) continue;

			const size_t cnt = static_cast<size_t>((tEnd - tBegin) / 8);
			if (cnt < 64 || cnt > 65536) continue;

			int valid = 0;
			const size_t sample = cnt < 16 ? cnt : 16;
			for (size_t i = 0; i < sample; ++i) {
				const uintptr_t ent = static_cast<uintptr_t>(readU64(static_cast<uintptr_t>(tBegin + i * 8)));
				if (IsPlausibleNativeEntry(hProc, ent)) ++valid;
			}
			if (valid < 2) continue;

			out.nativeTable = tableVA;
			out.entryCount  = cnt;
			out.valid       = true;
			return out;
		}

		return out;
	}
}
