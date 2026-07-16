#pragma once

#include <Core/SDK/Natives/CitizenNativeCore.hpp>
#include <Core/SDK/Natives/CrossmapNatives.hpp>
#include <Core/SDK/Natives/NativeHashNames.hpp>
#include <Core/SDK/Natives/Natives.hpp>
#include <Core/SDK/Natives/ShellcodeBuilder.hpp>
#include <Core/SDK/Memory.hpp>
#include <Core/SDK/DebugLog.hpp>
#include <Security/xorstr.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <initializer_list>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <Windows.h>

using namespace Core;

namespace NativeCaller {

	inline bool g_TraceInvoke = true;

	inline const char* NameOf(uint64_t hash) {
		const auto& m = Natives::HashToName();
		auto it = m.find(hash);
		return it == m.end() ? "?" : it->second.data();
	}

	constexpr size_t Q_TRIGGER   = 0x00;
	constexpr size_t Q_DONE      = 0x01;
	constexpr size_t Q_HANDLER   = 0x08;
	constexpr size_t Q_ARGCOUNT  = 0x18;
	constexpr size_t Q_ARGS      = 0x20;
	constexpr size_t Q_RESULT    = 0xC8;

	constexpr size_t E_HASH0     = 0x00;
	constexpr size_t E_HASH1     = 0x08;
	constexpr size_t E_HANDLER   = 0x18;

	struct CitizenEntry {
		uint64_t hash0;
		uint64_t hash1;
		uint64_t handler;
		uintptr_t slot;
	};

	class CNativeCaller {
		uintptr_t m_queueVA = 0;
		uintptr_t m_caveVA  = 0;
		uintptr_t m_slotVA  = 0;
		uintptr_t m_origFn  = 0;

		bool m_ready       = false;
		bool m_initFailed  = false;
		int  m_build       = 0;

		mutable std::mutex m_mtx;

		CitizenNativeCore::SCitizenCore m_core{};
		std::vector<CitizenEntry> m_citizen;
		std::unordered_map<uint64_t, uint64_t> m_citizenIndex;
		std::unordered_map<uint64_t, uint64_t> m_handlerCache;
		std::unordered_set<uint64_t> m_unresolvedHash;
		std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> m_coolDownUntil;
		std::vector<uint8_t> m_gameImage;
		bool m_gameImageTried = false;

		static constexpr std::chrono::milliseconds kCooldownAfterTimeout{1500};

	public:
		bool IsReady() const { return m_ready; }
		const CitizenNativeCore::SCitizenCore& GetCitizenCore() const { return m_core; }
		HANDLE   GetHookProc()        const { return Mem.ProcHandle; }
		DWORD    GetHookPid()         const { return Mem.ProcId; }
		uintptr_t GetQueueBase()      const { return m_queueVA; }
		uintptr_t GetAnchorSlotAddr() const { return m_slotVA; }

		bool EnsureReady() {
			if (m_ready) return true;
			if (m_initFailed) return false;
			Initialize();
			if (!m_ready) m_initFailed = true;
			return m_ready;
		}

		void Shutdown() {
			if (!m_ready) return;
			if (m_slotVA && m_origFn && Mem.ProcHandle)
				Mem.WriteProtected<uintptr_t>(m_slotVA, m_origFn);
			if (Mem.ProcHandle) {
				if (m_caveVA)  { VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_caveVA),  0, MEM_RELEASE); m_caveVA = 0; }
				if (m_queueVA) { VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_queueVA), 0, MEM_RELEASE); m_queueVA = 0; }
			}
			m_slotVA = m_origFn = 0;
			m_core = {};
			m_citizen.clear();
			m_citizenIndex.clear();
			m_handlerCache.clear();
			m_unresolvedHash.clear();
			m_coolDownUntil.clear();
			m_gameImage.clear();
			m_gameImage.shrink_to_fit();
			m_gameImageTried = false;
			m_ready = false;
		}

		uint64_t CitizenLookup(uint64_t hash) const {
			auto it = m_citizenIndex.find(hash);
			if (it != m_citizenIndex.end()) return it->second;

			const uint32_t low32 = static_cast<uint32_t>(hash & 0xFFFFFFFFULL);
			const uint64_t low64 = static_cast<uint64_t>(low32);
			it = m_citizenIndex.find(low64);
			if (it != m_citizenIndex.end()) return it->second;
			return 0;
		}

		uint64_t FollowStub(uintptr_t va) const {
			const uintptr_t base = Mem.ModBase;
			const uintptr_t end  = base + Mem.ModBaseSize;

			auto isCode = [base, end](uint64_t a) {
				return a >= 0x10000ULL && a < end;
			};
			auto isHandler = [&](uint64_t a) {
				if (!isCode(a)) return false;
				const uint8_t b = Mem.Read<uint8_t>(a);
				return b == 0x55 || b == 0x48 || b == 0x4C || b == 0x49 || b == 0xE9 || b == 0xEB;
			};

			uint64_t cur = va;
			std::set<uint64_t> seen;

			for (int hop = 0; hop < 3; ++hop) {
				if (seen.count(cur)) return 0;
				seen.insert(cur);

				uint8_t b[32]{};
				for (size_t i = 0; i < 32; ++i) b[i] = Mem.Read<uint8_t>(cur + i);

				if (b[0] == 0x41 && b[1] == 0x51 && b[2] == 0x4C && b[3] == 0x8D && b[4] == 0x0D) {
					const int32_t d = *reinterpret_cast<int32_t*>(b + 5);
					const uint64_t r = cur + 9ULL + static_cast<int64_t>(d);
					return isHandler(r) ? r : 0;
				}

				if (b[0] == 0xFF && b[1] == 0x25) {
					const int32_t d = *reinterpret_cast<int32_t*>(b + 2);
					const uint64_t p = cur + 6ULL + static_cast<int64_t>(d);
					const uint64_t t = Mem.Read<uint64_t>(p);
					return (t && isHandler(t)) ? t : 0;
				}

				if (b[0] == 0xE9) {
					const int32_t d = *reinterpret_cast<int32_t*>(b + 1);
					const uint64_t n = cur + 5ULL + static_cast<int64_t>(d);
					if (!isCode(n)) return 0;
					cur = n;
					continue;
				}

				if (b[0] == 0x48 && b[1] == 0xB8 && b[10] == 0xFF && b[11] == 0xE0) {
					const uint64_t t = *reinterpret_cast<uint64_t*>(b + 2);
					return isHandler(t) ? t : 0;
				}

				bool found = false;
				for (int off = 1; off <= 4 && !found; ++off) {
					if (b[off] == 0x4C && b[off + 1] == 0x8D && b[off + 2] == 0x0D) {
						const uint8_t prev = b[off - 1];
						const bool push = (prev >= 0x50 && prev <= 0x57) ||
										  (off >= 2 && b[off - 2] == 0x41 && prev >= 0x50 && prev <= 0x57);
						if (!push) continue;
						const int32_t d = *reinterpret_cast<int32_t*>(b + off + 3);
						const uint64_t r = cur + static_cast<uint64_t>(off + 7) + static_cast<int64_t>(d);
						if (isHandler(r)) return r;
						found = true;
					}
				}

				if (b[9] == 0xE9) {
					const int32_t d = *reinterpret_cast<int32_t*>(b + 10);
					const uint64_t n = cur + 14ULL + static_cast<int64_t>(d);
					if (isCode(n)) { cur = n; continue; }
				}

				return isHandler(cur) ? cur : 0;
			}
			return 0;
		}

		static int DetectBuild() {
			const std::wstring& n = Mem.ProcName;
			int build = 0;
			for (size_t i = 0; i + 2 < n.size(); ++i) {
				if (n[i] == L'_' && n[i + 1] == L'b' && n[i + 2] >= L'0' && n[i + 2] <= L'9') {
					for (size_t j = i + 2; j < n.size(); ++j) {
						if (n[j] >= L'0' && n[j] <= L'9')
							build = build * 10 + (n[j] - L'0');
						else if (n[j] == L'_')
							return build;
						else
							return 0;
					}
					return build;
				}
			}
			return 0;
		}

		struct CompiledPattern {
			std::vector<uint8_t> bytes;
			std::vector<uint8_t> mask;
		};

		static CompiledPattern CompilePattern(std::string_view src) {
			CompiledPattern out;
			size_t i = 0;
			while (i < src.size()) {
				while (i < src.size() && (src[i] == ' ' || src[i] == '\t')) ++i;
				if (i >= src.size()) break;

				if (src[i] == '?') {
					out.bytes.push_back(0);
					out.mask.push_back(0);
					while (i < src.size() && src[i] == '?') ++i;
					continue;
				}

				char buf[3] = { 0, 0, 0 };
				buf[0] = src[i++];
				if (i < src.size() && src[i] != ' ' && src[i] != '\t') buf[1] = src[i++];
				out.bytes.push_back(static_cast<uint8_t>(std::strtoul(buf, nullptr, 16)));
				out.mask.push_back(1);
			}
			return out;
		}

		bool EnsureGameImage() {
			if (m_gameImageTried) return !m_gameImage.empty();
			m_gameImageTried = true;
			if (!Mem.ProcHandle || !Mem.ModBase || !Mem.ModBaseSize) return false;

			m_gameImage.assign(Mem.ModBaseSize, 0);
			const size_t CHUNK = 4 * 1024 * 1024;
			size_t off = 0, readTotal = 0;
			while (off < Mem.ModBaseSize) {
				const size_t want = (Mem.ModBaseSize - off < CHUNK) ? (Mem.ModBaseSize - off) : CHUNK;
				SIZE_T got = 0;
				if (ReadProcessMemory(Mem.ProcHandle, reinterpret_cast<LPCVOID>(Mem.ModBase + off),
									  m_gameImage.data() + off, want, &got) && got > 0) {
					readTotal += got;
					off += got;
				} else {
					off += want;
				}
			}
			if (g_TraceInvoke)
				DebugLog(xorstr("[NC] game image: %.2f MB usable (of %.2f MB)\n"),
					readTotal / (1024.0 * 1024.0), Mem.ModBaseSize / (1024.0 * 1024.0));
			return readTotal > 0;
		}

		int64_t FindInGameImage(const CompiledPattern& p) const {
			const size_t n = p.bytes.size();
			if (!n || m_gameImage.size() < n) return -1;
			const uint8_t* img = m_gameImage.data();
			const size_t end = m_gameImage.size() - n;
			const uint8_t first = p.bytes[0];
			const bool firstMasked = p.mask[0] != 0;
			for (size_t i = 0; i <= end; ++i) {
				if (firstMasked && img[i] != first) continue;
				size_t j = 1;
				for (; j < n; ++j) {
					if (p.mask[j] && img[i + j] != p.bytes[j]) break;
				}
				if (j == n) return static_cast<int64_t>(i);
			}
			return -1;
		}

		uint64_t PatternResolve(uint64_t hash) {
			auto c = m_handlerCache.find(hash);
			if (c != m_handlerCache.end()) return c->second;

			if (!m_build) m_build = DetectBuild();

			const auto& hashMap = Natives::HashToName();
			const auto n = hashMap.find(hash);
			if (n == hashMap.end()) {
				if (g_TraceInvoke) DebugLog(xorstr("[NC] pattern: 0x%llX unknown hash\n"), (unsigned long long)hash);
				m_handlerCache[hash] = 0; return 0;
			}

			const std::string_view* pat = Natives::findPatternForBuild(n->second, m_build);
			if (!pat || pat->empty()) {
				if (g_TraceInvoke) DebugLog(xorstr("[NC] pattern: %s no signature for build %d\n"), n->second.data(), m_build);
				m_handlerCache[hash] = 0; return 0;
			}

			if (!EnsureGameImage()) {
				if (g_TraceInvoke) DebugLog(xorstr("[NC] pattern: %s game image unavailable\n"), n->second.data());
				m_handlerCache[hash] = 0; return 0;
			}

			const CompiledPattern compiled = CompilePattern(*pat);
			const int64_t off = FindInGameImage(compiled);
			if (off < 0) {
				if (g_TraceInvoke) DebugLog(xorstr("[NC] pattern: %s signature miss (len=%zu)\n"), n->second.data(), compiled.bytes.size());
				m_handlerCache[hash] = 0; return 0;
			}

			const uintptr_t hitVA = Mem.ModBase + static_cast<uintptr_t>(off);
			const uint64_t handler = FollowStub(hitVA);
			m_handlerCache[hash] = (handler > 0x10000ULL) ? handler : 0;
			if (g_TraceInvoke) {
				if (m_handlerCache[hash])
					DebugLog(xorstr("[NC] pattern: %s -> handler=0x%llX (hit=0x%llX)\n"), n->second.data(), (unsigned long long)m_handlerCache[hash], (unsigned long long)hitVA);
				else
					DebugLog(xorstr("[NC] pattern: %s FollowStub failed (hit=0x%llX)\n"), n->second.data(), (unsigned long long)hitVA);
			}
			return m_handlerCache[hash];
		}

		template<typename T>
		static uint64_t PackArg(T value) {
			using U = std::decay_t<T>;
			if constexpr (std::is_same_v<U, float>) {
				uint64_t bits = 0; std::memcpy(&bits, &value, sizeof(value)); return bits;
			} else if constexpr (std::is_same_v<U, double>) {
				uint64_t bits = 0; std::memcpy(&bits, &value, sizeof(value)); return bits;
			} else if constexpr (std::is_same_v<U, bool>) {
				return value ? 1ULL : 0ULL;
			} else if constexpr (std::is_same_v<U, uint64_t>) {
				return value;
			} else if constexpr (std::is_same_v<U, int64_t>) {
				return static_cast<uint64_t>(value);
			} else if constexpr (std::is_pointer_v<U>) {
				return reinterpret_cast<uint64_t>(value);
			} else if constexpr (std::is_integral_v<U>) {
				return static_cast<uint64_t>(static_cast<int64_t>(value));
			} else if constexpr (std::is_enum_v<U>) {
				return static_cast<uint64_t>(static_cast<std::underlying_type_t<U>>(value));
			} else {
				return static_cast<uint64_t>(value);
			}
		}

		static bool IsNetworkNative(uint64_t hash) {
			const auto& hashMap = Natives::HashToName();
			const auto it = hashMap.find(hash);
			if (it == hashMap.end()) return false;
			std::string_view name = it->second;
			return name.size() >= 8 && std::memcmp(name.data(), "NETWORK_", 8) == 0;
		}

		uint64_t InvokeRaw(uintptr_t queue, uint64_t hash,
						   const std::vector<uint64_t>& args, int timeoutMs) {
			const char* name = NameOf(hash);

			if (!queue) return 0;
			if (args.size() > 8) return 0;

			if (m_unresolvedHash.count(hash)) return 0;

			{
				auto cd = m_coolDownUntil.find(hash);
				if (cd != m_coolDownUntil.end()) {
					if (std::chrono::steady_clock::now() < cd->second) return 0;
					m_coolDownUntil.erase(cd);
				}
			}

			const char* src = "citizen";
			uint64_t handler = CitizenLookup(hash);
			if (!handler) {
				handler = PatternResolve(hash);
				src = "pattern";
			}
			if (!handler) {
				if (g_TraceInvoke) DebugLog(xorstr("[NC] %s(0x%llX) UNRESOLVED\n"), name, (unsigned long long)hash);
				m_unresolvedHash.insert(hash);
				return 0;
			}

			if (g_TraceInvoke) {
				char argStr[192] = "";
				size_t off = 0;
				for (size_t i = 0; i < args.size() && off + 20 < sizeof(argStr); ++i) {
					int w = _snprintf_s(argStr + off, sizeof(argStr) - off, _TRUNCATE,
						i ? ", 0x%llX" : "0x%llX", (unsigned long long)args[i]);
					if (w <= 0) break;
					off += w;
				}
				DebugLog(xorstr("[NC] -> %s [%s] handler=0x%llX args=[%s]\n"),
					name, src, (unsigned long long)handler, argStr);
			}

			uint64_t argBuf[8]{};
			for (size_t i = 0; i < args.size(); ++i) argBuf[i] = args[i];
			Mem.WriteRaw(queue + Q_ARGS, argBuf, sizeof(argBuf));

			uint8_t clearResult[24]{};
			Mem.WriteRaw(queue + Q_RESULT, clearResult, sizeof(clearResult));

			Mem.Write<uint8_t>(queue + Q_DONE, 0);
			Mem.Write<uint64_t>(queue + Q_HANDLER, handler);
			Mem.Write<uint32_t>(queue + Q_ARGCOUNT, static_cast<uint32_t>(args.size()));
			Mem.Write<uint8_t>(queue + Q_TRIGGER, 1);

			const auto start    = std::chrono::steady_clock::now();
			const auto deadline = start + std::chrono::milliseconds(timeoutMs);

			while (std::chrono::steady_clock::now() < deadline) {
				if (Mem.Read<uint8_t>(queue + Q_DONE)) {
					const uint64_t r = Mem.Read<uint64_t>(queue + Q_RESULT);
					if (g_TraceInvoke) {
						const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - start).count();
						DebugLog(xorstr("[NC] <- %s = 0x%llX (%lldms)\n"),
							name, (unsigned long long)r, (long long)ms);
					}
					return r;
				}
				std::this_thread::sleep_for(std::chrono::microseconds(500));
			}

			if (g_TraceInvoke) {
				DebugLog(xorstr("[NC] !! %s TIMEOUT (%dms) - cooldown %dms\n"),
					name, timeoutMs, (int)kCooldownAfterTimeout.count());
			}
			m_coolDownUntil[hash] = std::chrono::steady_clock::now() + kCooldownAfterTimeout;
			return 0;
		}

		uint64_t Invoke(uint64_t hash, std::initializer_list<uint64_t> args = {},
						int timeoutMs = 400) {
			std::lock_guard<std::mutex> lk(m_mtx);
			if (!m_ready) return 0;
			const std::vector<uint64_t> v(args.begin(), args.end());
			return InvokeRaw(m_queueVA, hash, v, timeoutMs);
		}

		template<typename First, typename... Rest>
		std::enable_if_t<!std::is_same_v<std::decay_t<First>, std::initializer_list<uint64_t>>, uint64_t>
		Invoke(uint64_t hash, First first, Rest... rest) {
			std::lock_guard<std::mutex> lk(m_mtx);
			if (!m_ready) return 0;

			std::vector<uint64_t> packed;
			packed.reserve(1 + sizeof...(Rest));
			packed.push_back(PackArg(first));
			(packed.push_back(PackArg(rest)), ...);
			return InvokeRaw(m_queueVA, hash, packed, 400);
		}

		bool Probe(int timeoutMs = 2500) {
			if (!m_ready || !m_queueVA || !m_caveVA) return false;
			std::lock_guard<std::mutex> lk(m_mtx);
			const std::vector<uint64_t> no;
			return InvokeRaw(m_queueVA, Natives::PLAYER_PED_ID, no, timeoutMs) != 0;
		}

		void Initialize() {
			if (m_ready) return;

			if (!Mem.ProcId || !Mem.ProcHandle) {
				DebugLog(xorstr("NativeCaller: process not open\n"));
				return;
			}

			m_core = CitizenNativeCore::Resolve(Mem.ProcHandle, Mem.ProcId);
			if (!m_core.valid) {
				DebugLog(xorstr("NativeCaller: citizen core resolve failed\n"));
				return;
			}

			const uintptr_t entriesBegin = Mem.Read<uintptr_t>(m_core.nativeTable);
			const uintptr_t entriesEnd   = Mem.Read<uintptr_t>(m_core.nativeTable + 8);
			if (entriesEnd <= entriesBegin) return;
			const size_t entryCount = (entriesEnd - entriesBegin) / 8;
			if (entryCount < 64) {
				DebugLog(xorstr("NativeCaller: entry count too small (%zu)\n"), entryCount);
				return;
			}

			m_citizen.clear();
			m_citizen.reserve(entryCount);
			m_citizenIndex.clear();
			m_citizenIndex.reserve(entryCount * 2);

			const uintptr_t base = Mem.ModBase;
			const uintptr_t end  = base + Mem.ModBaseSize;

			std::vector<std::pair<uintptr_t, uintptr_t>> anchors;
			for (size_t i = 0; i < entryCount; ++i) {
				const uintptr_t ent = Mem.Read<uintptr_t>(entriesBegin + i * 8);
				if (!ent || ent < 0x10000ULL) continue;
				if (!CitizenNativeCore::IsPlausibleNativeEntry(Mem.ProcHandle, ent)) continue;

				CitizenEntry ce{};
				ce.hash0   = Mem.Read<uint64_t>(ent + E_HASH0);
				ce.hash1   = Mem.Read<uint64_t>(ent + E_HASH1);
				ce.handler = Mem.Read<uint64_t>(ent + E_HANDLER);
				ce.slot    = ent + E_HANDLER;
				m_citizen.push_back(ce);

				if (ce.handler > 0x10000ULL) {
					m_citizenIndex.emplace(ce.hash0, ce.handler);
					m_citizenIndex.emplace(ce.hash1, ce.handler);
					m_citizenIndex.emplace(ce.hash0 & 0xFFFFFFFFULL, ce.handler);
					m_citizenIndex.emplace(ce.hash1 & 0xFFFFFFFFULL, ce.handler);
				}

				if (ce.handler >= base && ce.handler < end)
					anchors.emplace_back(ce.slot, static_cast<uintptr_t>(ce.handler));
			}

			const uint64_t ggtHandler = CitizenLookup(Natives::GET_GAME_TIMER);
			if (!ggtHandler) {
				DebugLog(xorstr("NativeCaller: GET_GAME_TIMER not in citizen table\n"));
				return;
			}
			if (anchors.empty()) {
				DebugLog(xorstr("NativeCaller: no anchor candidates\n"));
				return;
			}

			const auto sc = ShellcodeBuilder::buildCitizenShellcode();
			const size_t scLen = sc.buffer.size();

			uintptr_t queueVA = 0, caveVA = 0, foundSlot = 0, foundOrig = 0;

			for (const auto& [slot, orig] : anchors) {
				if (queueVA) { VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(queueVA), 0, MEM_RELEASE); queueVA = 0; }
				if (caveVA)  { VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(caveVA),  0, MEM_RELEASE); caveVA  = 0; }

				queueVA = reinterpret_cast<uintptr_t>(
					VirtualAllocEx(Mem.ProcHandle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
				caveVA = reinterpret_cast<uintptr_t>(
					VirtualAllocEx(Mem.ProcHandle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
				if (!queueVA || !caveVA) continue;

				std::vector<uint8_t> code = sc.buffer;
				std::memcpy(code.data() + sc.PATCH_QUEUE,    &queueVA, 8);
				std::memcpy(code.data() + sc.PATCH_ORIGFUNC, &orig,    8);

				if (!Mem.WriteRaw(caveVA, code.data(), scLen)) continue;
				if (!Mem.WriteProtected<uintptr_t>(slot, caveVA)) continue;

				Mem.Write<uint8_t>(queueVA + Q_TRIGGER, 0);
				Mem.Write<uint8_t>(queueVA + Q_DONE,    0);
				Mem.Write<uint64_t>(queueVA + Q_HANDLER,  ggtHandler);
				Mem.Write<uint32_t>(queueVA + Q_ARGCOUNT, 0);
				Mem.Write<uint8_t>(queueVA + Q_TRIGGER, 1);

				bool ok = false;
				for (int w = 0; w < 10; ++w) {
					Sleep(10);
					if (Mem.Read<uint8_t>(queueVA + Q_DONE)) {
						ok = Mem.Read<uint64_t>(queueVA + Q_RESULT) != 0;
						break;
					}
				}

				Mem.WriteProtected<uintptr_t>(slot, orig);

				if (ok) {
					foundSlot = slot;
					foundOrig = orig;
					DebugLog(xorstr("NativeCaller: anchor OK @ slot 0x%p\n"), (void*)slot);
					break;
				}
			}

			if (!foundSlot) {
				if (queueVA) VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(queueVA), 0, MEM_RELEASE);
				if (caveVA)  VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(caveVA),  0, MEM_RELEASE);
				DebugLog(xorstr("NativeCaller: no working anchor\n"));
				return;
			}

			std::vector<uint8_t> finalCode = sc.buffer;
			std::memcpy(finalCode.data() + sc.PATCH_QUEUE,    &queueVA,   8);
			std::memcpy(finalCode.data() + sc.PATCH_ORIGFUNC, &foundOrig, 8);
			Mem.WriteRaw(caveVA, finalCode.data(), scLen);
			Mem.WriteProtected<uintptr_t>(foundSlot, caveVA);

			m_queueVA = queueVA;
			m_caveVA  = caveVA;
			m_slotVA  = foundSlot;
			m_origFn  = foundOrig;
			m_build   = DetectBuild();
			m_ready   = true;

			DebugLog(xorstr("NativeCaller: active build=%d\n"), m_build);
		}
	};

	inline CNativeCaller g_NativeCaller;
}

using NativeCaller::g_NativeCaller;
