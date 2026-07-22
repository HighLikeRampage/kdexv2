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

		auto tryTableAt = [&](uintptr_t tableVA) -> bool {
			const uint64_t tBegin = readU64(tableVA);
			const uint64_t tEnd   = readU64(tableVA + 8);
			if (!tBegin || !tEnd || tEnd <= tBegin) return false;
			if (tBegin < 0x10000ULL || tBegin > 0x7FFF00000000ULL) return false;
			if (tEnd   < 0x10000ULL || tEnd   > 0x7FFF00000000ULL) return false;
			const uint64_t diff = tEnd - tBegin;
			if (diff % 8 != 0) return false;
			const size_t cnt = static_cast<size_t>(diff / 8);
			if (cnt < 64 || cnt > 65536) return false;
			const size_t sample = cnt < 16 ? cnt : 16;
			int valid = 0;
			for (size_t i = 0; i < sample; ++i) {
				const uintptr_t ent = static_cast<uintptr_t>(readU64(static_cast<uintptr_t>(tBegin + i * 8)));
				if (IsPlausibleNativeEntry(hProc, ent)) ++valid;
			}
			if (valid < 2) return false;
			out.nativeTable = tableVA;
			out.entryCount  = cnt;
			out.valid       = true;
			return true;
		};

		for (const auto& fb : KnownTableOffsets()) {
			if (tryTableAt(coreBase + fb.table))
				return out;
		}

		// Full-module scan fallback for builds not in KnownTableOffsets.
		// Read citizen-scripting-core.dll in 4 MB chunks and look for a
		// {ptr begin, ptr end} pair whose pointed-to array passes sample
		// validation as a native handler table.
		{
			const size_t CHUNK = 4 * 1024 * 1024;
			std::vector<uint8_t> buf;
			size_t scanOff = 0;
			while (scanOff + 16 <= coreSize) {
				const size_t want = ((coreSize - scanOff) < CHUNK)
				                    ? (coreSize - scanOff) : CHUNK;
				buf.assign(want, 0);
				SIZE_T got = 0;
				if (!ReadProcessMemory(hProc,
				    reinterpret_cast<LPCVOID>(coreBase + scanOff),
				    buf.data(), want, &got) || got < 16) {
					scanOff += want; continue;
				}
				for (size_t i = 0; i + 16 <= got; i += 8) {
					uint64_t tBegin = 0, tEnd = 0;
					std::memcpy(&tBegin, buf.data() + i,     8);
					std::memcpy(&tEnd,   buf.data() + i + 8, 8);
					if (!tBegin || !tEnd || tEnd <= tBegin) continue;
					if (tBegin < 0x10000ULL || tBegin > 0x7FFF00000000ULL) continue;
					if (tEnd   < 0x10000ULL || tEnd   > 0x7FFF00000000ULL) continue;
					const uint64_t diff = tEnd - tBegin;
					if (diff % 8 != 0) continue;
					const size_t cnt = static_cast<size_t>(diff / 8);
					if (cnt < 64 || cnt > 65536) continue;
					const size_t sample = cnt < 16 ? cnt : 16;
					int valid = 0;
					for (size_t j = 0; j < sample; ++j) {
						const uintptr_t ent = static_cast<uintptr_t>(
						    readU64(static_cast<uintptr_t>(tBegin + j * 8)));
						if (IsPlausibleNativeEntry(hProc, ent)) ++valid;
					}
					if (valid < 6) continue;
					// Extended validation to reduce false positives
					const size_t ext = cnt < 32 ? cnt : 32;
					int extValid = 0;
					for (size_t j = 0; j < ext; ++j) {
						const uintptr_t ent = static_cast<uintptr_t>(
						    readU64(static_cast<uintptr_t>(tBegin + j * 8)));
						if (IsPlausibleNativeEntry(hProc, ent)) ++extValid;
					}
					if (extValid * 2 < static_cast<int>(ext)) continue;
					out.nativeTable = coreBase + scanOff + i;
					out.entryCount  = cnt;
					out.valid       = true;
					return out;
				}
				scanOff += want;
			}
		}

		return out;
	}
}
