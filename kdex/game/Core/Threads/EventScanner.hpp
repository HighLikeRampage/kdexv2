#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <Includes/Includes.hpp>
#include <Security/xorstr.hpp>

namespace Core
{
namespace Threads
{

struct CapturedEvent {
	uint16_t id = 0;
	uint32_t count = 0;
	uint32_t blockedCount = 0;
	uintptr_t lastAddr = 0;
};

class cEventScanner
{
public:
	~cEventScanner()
	{
		m_running.store(false);
		if (m_thread.joinable()) m_thread.join();
	}

	void Start()
	{
		if (m_running.load()) return;
		if (m_thread.joinable()) m_thread.join();
		m_running.store(true);
		m_thread = std::thread(&cEventScanner::ScanLoop, this);
	}

	void Stop()
	{
		m_running.store(false);
		if (m_thread.joinable()) m_thread.join();
	}

	bool IsRunning() const { return m_running.load(); }

	void Clear()
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		m_events.clear();
	}

	void ToggleBlock(uint16_t id)
	{
		std::lock_guard<std::mutex> lk(m_blockMutex);
		if (m_blocked.count(id))
			m_blocked.erase(id);
		else
			m_blocked.insert(id);
	}

	bool IsBlocked(uint16_t id)
	{
		std::lock_guard<std::mutex> lk(m_blockMutex);
		return m_blocked.count(id) > 0;
	}

	std::vector<CapturedEvent> GetEvents()
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		std::vector<CapturedEvent> out;
		out.reserve(m_events.size());
		for (auto& p : m_events)
			out.push_back(p.second);
		std::sort(out.begin(), out.end(), [](const CapturedEvent& a, const CapturedEvent& b) {
			return a.count > b.count;
		});
		return out;
	}

	static const char* EventName(uint16_t id)
	{
		static const std::unordered_map<uint16_t, const char*> names = {
			{3,"SCRIPT_ARRAY_DATA_VERIFY"},{4,"REQUEST_CONTROL"},{5,"GIVE_CONTROL"},
			{6,"WEAPON_DAMAGE"},{7,"REQUEST_PICKUP"},{8,"REQUEST_MAP_PICKUP"},
			{11,"RESPAWN_PLAYER_PED"},{12,"GIVE_WEAPON"},{13,"REMOVE_WEAPON"},
			{14,"REMOVE_ALL_WEAPONS"},{15,"VEHICLE_COMPONENT_CONTROL"},
			{16,"FIRE"},{17,"EXPLOSION"},{18,"START_PROJECTILE"},
			{19,"UPDATE_PROJECTILE_TARGET"},{20,"REMOVE_PROJECTILE_ENTITY"},
			{21,"BREAK_PROJECTILE_TARGET_LOCK"},{22,"ALTER_WANTED_LEVEL"},
			{23,"CHANGE_RADIO_STATION"},{24,"RAGDOLL_REQUEST"},{25,"PLAYER_TAUNT"},
			{26,"PLAYER_CARD_STAT"},{27,"DOOR_BREAK"},{28,"SCRIPTED_GAME"},
			{29,"REMOTE_SCRIPT_INFO"},{30,"REMOTE_SCRIPT_LEAVE"},
			{31,"MARK_AS_NO_LONGER_NEEDED"},{32,"CONVERT_TO_SCRIPT_ENTITY"},
			{33,"SCRIPT_WORLD_STATE"},{34,"CLEAR_AREA"},{35,"CLEAR_RECTANGLE_AREA"},
			{36,"NET_REQUEST_SYNCED_SCENE"},{37,"NET_START_SYNCED_SCENE"},
			{38,"NET_STOP_SYNCED_SCENE"},{39,"NET_UPDATE_SYNCED_SCENE"},
			{40,"INCIDENT_ENTITY"},{41,"GIVE_PED_SCRIPTED_TASK"},
			{42,"GIVE_PED_SEQUENCE_TASK"},{43,"NET_CLEAR_PED_TASKS"},
			{44,"NET_START_PED_ARREST"},{45,"NET_START_PED_UNCUFF"},
			{46,"NET_SOUND_CAR_HORN"},{47,"NET_ENTITY_AREA_STATUS"},
			{48,"NET_GARAGE_OCCUPIED_STATUS"},{49,"PED_CONVERSATION_LINE"},
			{50,"SCRIPT_ENTITY_STATE_CHANGE"},{51,"NET_PLAY_SOUND"},
			{52,"NET_STOP_SOUND"},{53,"NET_PLAY_AIRDEFENSE_FIRE"},
			{54,"NET_BANK_REQUEST"},{55,"NET_AUDIO_BARK"},
			{56,"REQUEST_DOOR"},{57,"NET_TRAIN_REPORT"},{58,"NET_TRAIN_REQUEST"},
			{59,"NET_INCREMENT_STAT"},{60,"MODIFY_VEHICLE_LOCK_WORLD_STATE"},
			{61,"MODIFY_PTFX_WORLD_STATE"},{62,"REQUEST_PHONE_EXPLOSION"},
			{63,"REQUEST_DETACHMENT"},{64,"KICK_VOTES"},{65,"GIVE_PICKUP_REWARDS"},
			{66,"BLOW_UP_VEHICLE"},{67,"NET_SPECIAL_FIRE_EQUIPPED_WEAPON"},
			{68,"NET_RESPONDED_TO_THREAT"},{69,"NET_SHOUT_TARGET_POSITION"},
			{70,"VOICE_MOUTH_MOVEMENT_FINISHED"},{71,"PICKUP_DESTROYED"},
			{72,"UPDATE_PLAYER_SCARS"},{73,"NET_CHECK_EXE_SIZE"},
			{74,"NET_PTFX"},{75,"NET_PED_SEEN_DEAD_PED"},{76,"REMOVE_STICKY_BOMB"},
			{77,"NET_CHECK_CODE_CRCS"},{78,"INFORM_SILENCED_GUNSHOT"},
			{79,"PED_PLAY_PAIN"},{80,"CACHE_PLAYER_HEAD_BLEND_DATA"},
			{81,"REMOVE_PED_FROM_PEDGROUP"},{82,"REPORT_MYSELF"},
			{83,"REPORT_CASH_SPAWN"},{84,"ACTIVATE_VEHICLE_SPECIAL_ABILITY"},
			{85,"BLOCK_WEAPON_SELECTION"},{86,"NET_CHECK_CATALOG_CRC"}
		};
		auto it = names.find(id);
		return it != names.end() ? it->second : "UNKNOWN";
	}

private:
	struct ModRange { uintptr_t base, end; };

	std::atomic<bool> m_running{ false };
	std::thread m_thread;

	std::mutex m_mutex;
	std::unordered_map<uint16_t, CapturedEvent> m_events;

	std::mutex m_blockMutex;
	std::unordered_set<uint16_t> m_blocked;

	static bool IsValid(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFF0000ULL; }

	static bool IsInAnyModule(uintptr_t addr, const std::vector<ModRange>& mods)
	{
		for (auto& m : mods)
			if (addr >= m.base && addr < m.end) return true;
		return false;
	}

	void HeapScan(HANDLE hProc, const std::vector<ModRange>& allMods, std::unordered_set<uintptr_t>& seenHeap)
	{
		MEMORY_BASIC_INFORMATION mbi;
		uintptr_t addr = 0;

		while (VirtualQueryEx(hProc, (LPCVOID)addr, &mbi, sizeof(mbi))) {
			uintptr_t regionEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
			if (regionEnd <= addr) break;
			addr = regionEnd;

			if (mbi.State != MEM_COMMIT) continue;
			if (mbi.Type != MEM_PRIVATE) continue;
			if (!(mbi.Protect & PAGE_READWRITE)) continue;
			if (mbi.RegionSize > 64 * 1024 * 1024) continue;

			std::vector<uint8_t> buf(mbi.RegionSize);
			SIZE_T n = 0;
			if (!ReadProcessMemory(hProc, mbi.BaseAddress, buf.data(), mbi.RegionSize, &n)) continue;

			for (size_t i = 0; i + 16 <= n; i += 8) {
				uintptr_t vtable = *(uintptr_t*)(buf.data() + i);
				if (!IsInAnyModule(vtable, allMods)) continue;

				uint16_t evType = *(uint16_t*)(buf.data() + i + 8);
				if (evType == 0 || evType > 200) continue;

				uintptr_t objAddr = (uintptr_t)mbi.BaseAddress + i;
				if (seenHeap.count(objAddr)) continue;
				seenHeap.insert(objAddr);

				bool blocked = CheckBlocked(evType);
				if (blocked) BlockEvent(hProc, objAddr);

				RecordEvent(evType, objAddr, blocked);
			}
		}
	}

	uintptr_t FindModuleBase(DWORD pid, const wchar_t* name, DWORD& outSize)
	{
		outSize = 0;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (snap == INVALID_HANDLE_VALUE) return 0;
		MODULEENTRY32W me = { sizeof(me) };
		uintptr_t result = 0;
		if (Module32FirstW(snap, &me)) {
			do {
				if (_wcsicmp(me.szModule, name) == 0) {
					result = (uintptr_t)me.modBaseAddr;
					outSize = me.modBaseSize;
					break;
				}
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
		return result;
	}

	std::vector<ModRange> GetAllModules(DWORD pid)
	{
		std::vector<ModRange> mods;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (snap == INVALID_HANDLE_VALUE) return mods;
		MODULEENTRY32W me = { sizeof(me) };
		if (Module32FirstW(snap, &me)) {
			do {
				mods.push_back({ (uintptr_t)me.modBaseAddr, (uintptr_t)me.modBaseAddr + me.modBaseSize });
			} while (Module32NextW(snap, &me));
		}
		CloseHandle(snap);
		return mods;
	}

	void RecordEvent(uint16_t evType, uintptr_t addr, bool wasBlocked)
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		auto& ev = m_events[evType];
		if (ev.id == 0) ev.id = evType;
		ev.count++;
		ev.lastAddr = addr;
		if (wasBlocked) ev.blockedCount++;
	}

	void BlockEvent(HANDLE hProc, uintptr_t dataPtr)
	{
		uint16_t zero = 0;
		uint8_t zeroBytes[48] = {};
		SIZE_T n = 0;
		WriteProcessMemory(hProc, (LPVOID)(dataPtr + 0x08), &zero, sizeof(zero), &n);
		WriteProcessMemory(hProc, (LPVOID)(dataPtr + 0x10), zeroBytes, sizeof(zeroBytes), &n);
	}

	bool CheckBlocked(uint16_t evType)
	{
		std::lock_guard<std::mutex> lk(m_blockMutex);
		return m_blocked.count(evType) > 0;
	}

	void TraverseTree(HANDLE hProc, uintptr_t sentinel, int nilOffset,
		std::unordered_set<uintptr_t>& seen, const std::vector<ModRange>& allMods)
	{
		if (!IsValid(sentinel)) return;

		std::queue<uintptr_t> bfs;
		std::unordered_set<uintptr_t> visited;
		bfs.push(sentinel);
		visited.insert(sentinel);

		while (!bfs.empty() && visited.size() < 50000) {
			uintptr_t node = bfs.front();
			bfs.pop();
			if (!IsValid(node)) continue;

			for (int p = 0; p < 3; p++) {
				uintptr_t child = 0;
				SIZE_T n = 0;
				if (ReadProcessMemory(hProc, (LPCVOID)(node + (uintptr_t)p * 8), &child, 8, &n)
					&& IsValid(child) && !visited.count(child)) {
					visited.insert(child);
					bfs.push(child);
				}
			}

			if (nilOffset >= 0) {
				uint8_t isNil = 0;
				SIZE_T n = 0;
				if (!ReadProcessMemory(hProc, (LPCVOID)(node + nilOffset), &isNil, 1, &n) || isNil != 0)
					continue;
			}

			if (seen.count(node)) continue;

			for (int dOff : {0x28, 0x20, 0x30}) {
				uintptr_t dataPtr = 0;
				SIZE_T n = 0;
				if (!ReadProcessMemory(hProc, (LPCVOID)(node + dOff), &dataPtr, 8, &n) || !IsValid(dataPtr))
					continue;

				uintptr_t vtable = 0;
				if (!ReadProcessMemory(hProc, (LPCVOID)dataPtr, &vtable, 8, &n))
					continue;
				if (!IsInAnyModule(vtable, allMods)) continue;

				uint16_t evType = 0;
				if (!ReadProcessMemory(hProc, (LPCVOID)(dataPtr + 0x08), &evType, 2, &n) || evType == 0 || evType > 200)
					continue;

				seen.insert(node);

				bool blocked = CheckBlocked(evType);
				if (blocked) BlockEvent(hProc, dataPtr);

				RecordEvent(evType, dataPtr, blocked);
				break;
			}
		}
	}

	bool TryKnownOffsets(HANDLE hProc, uintptr_t dllBase, std::vector<uintptr_t>& roots, int& nilOffset)
	{
		const uintptr_t KNOWN[] = { 0x2003C0, 0x2003D0 };
		for (auto off : KNOWN) {
			uintptr_t ptr = 0;
			SIZE_T n = 0;
			if (!ReadProcessMemory(hProc, (LPCVOID)(dllBase + off), &ptr, 8, &n) || !IsValid(ptr))
				continue;

			for (int tryNil : {0x19, 0x11}) {
				uint8_t nil = 0;
				if (ReadProcessMemory(hProc, (LPCVOID)(ptr + tryNil), &nil, 1, &n) && nil == 1) {
					roots.push_back(off);
					nilOffset = tryNil;
					break;
				}
			}

			if (roots.empty()) {
				uintptr_t p0 = 0, p1 = 0, p2 = 0;
				ReadProcessMemory(hProc, (LPCVOID)ptr, &p0, 8, &n);
				ReadProcessMemory(hProc, (LPCVOID)(ptr + 8), &p1, 8, &n);
				ReadProcessMemory(hProc, (LPCVOID)(ptr + 16), &p2, 8, &n);
				if (p0 == ptr && p1 == ptr && p2 == ptr)
					roots.push_back(off);
			}
		}
		return !roots.empty();
	}

	bool ScanDataSection(HANDLE hProc, uintptr_t dllBase, DWORD dllSize,
		const std::vector<ModRange>& allMods, std::vector<uintptr_t>& roots, int& nilOffset)
	{
		IMAGE_DOS_HEADER dos = {};
		SIZE_T n = 0;
		if (!ReadProcessMemory(hProc, (LPCVOID)dllBase, &dos, sizeof(dos), &n) || dos.e_magic != IMAGE_DOS_SIGNATURE)
			return false;

		IMAGE_NT_HEADERS64 nt = {};
		if (!ReadProcessMemory(hProc, (LPCVOID)(dllBase + dos.e_lfanew), &nt, sizeof(nt), &n))
			return false;

		uintptr_t secAddr = dllBase + dos.e_lfanew + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) + nt.FileHeader.SizeOfOptionalHeader;

		for (int i = 0; i < nt.FileHeader.NumberOfSections; i++) {
			IMAGE_SECTION_HEADER sec = {};
			if (!ReadProcessMemory(hProc, (LPCVOID)(secAddr + i * sizeof(sec)), &sec, sizeof(sec), &n))
				continue;
			if (memcmp(sec.Name, ".data", 5) != 0) continue;

			DWORD dataSize = sec.Misc.VirtualSize;
			uintptr_t dataRVA = sec.VirtualAddress;

			std::vector<uint8_t> buf(dataSize);
			SIZE_T bytesRead = 0;
			if (!ReadProcessMemory(hProc, (LPCVOID)(dllBase + dataRVA), buf.data(), dataSize, &bytesRead))
				return false;

			for (DWORD off = 0; off + 16 <= (DWORD)bytesRead; off += 8) {
				uintptr_t ptr = *(uintptr_t*)(buf.data() + off);
				if (!IsValid(ptr)) continue;

				uint8_t targetBytes[0x30] = {};
				SIZE_T tbRead = 0;
				if (!ReadProcessMemory(hProc, (LPCVOID)ptr, targetBytes, sizeof(targetBytes), &tbRead) || tbRead < 0x20)
					continue;

				uintptr_t p0 = *(uintptr_t*)(targetBytes + 0x00);
				uintptr_t p1 = *(uintptr_t*)(targetBytes + 0x08);
				uintptr_t p2 = *(uintptr_t*)(targetBytes + 0x10);

				bool isSelfRef = (p0 == ptr && p1 == ptr && p2 == ptr);
				bool hasIsNil19 = (targetBytes[0x19] == 1);
				bool hasIsNil11 = (targetBytes[0x11] == 1);

				if (!isSelfRef && !hasIsNil19 && !hasIsNil11) continue;

				if (isSelfRef) {
					roots.push_back(dataRVA + off);
					continue;
				}

				uintptr_t children[] = { p0, p1, p2 };
				for (int c = 0; c < 3; c++) {
					if (!IsValid(children[c]) || children[c] == ptr) continue;

					int testNil = hasIsNil19 ? 0x19 : (hasIsNil11 ? 0x11 : -1);
					if (testNil >= 0) {
						uint8_t childNil = 0;
						if (!ReadProcessMemory(hProc, (LPCVOID)(children[c] + testNil), &childNil, 1, &n) || childNil != 0)
							continue;
					}

					for (int dOff : {0x28, 0x20, 0x30}) {
						uintptr_t dataPtr = 0;
						if (!ReadProcessMemory(hProc, (LPCVOID)(children[c] + dOff), &dataPtr, 8, &n) || !IsValid(dataPtr))
							continue;

						uintptr_t vtable = 0;
						if (!ReadProcessMemory(hProc, (LPCVOID)dataPtr, &vtable, 8, &n))
							continue;

						if (IsInAnyModule(vtable, allMods)) {
							uint16_t evType = 0;
							if (ReadProcessMemory(hProc, (LPCVOID)(dataPtr + 0x08), &evType, 2, &n) && evType <= 200) {
								roots.push_back(dataRVA + off);
								nilOffset = testNil;
								goto done_scan;
							}
						}
					}
				}
			}
			done_scan:
			break;
		}
		return !roots.empty();
	}

	void ScanLoop()
	{
		DWORD pid = g_Variables.ProcIdFiveM;
		if (pid == 0) {
			m_running.store(false);
			return;
		}

		HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
		if (!hProc) {
			m_running.store(false);
			return;
		}

		DWORD dllSize = 0;
		uintptr_t dllBase = 0;

		for (int attempt = 0; attempt < 30 && m_running.load(); attempt++) {
			dllBase = FindModuleBase(pid, L"gta-net-five.dll", dllSize);
			if (dllBase) break;
			for (int s = 0; s < 10 && m_running.load(); s++)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		if (!dllBase) {
			CloseHandle(hProc);
			m_running.store(false);
			return;
		}

		auto allMods = GetAllModules(pid);
		std::vector<uintptr_t> roots;
		int nilOffset = 0x19;
		bool foundTrees = TryKnownOffsets(hProc, dllBase, roots, nilOffset);

		if (!foundTrees)
			foundTrees = ScanDataSection(hProc, dllBase, dllSize, allMods, roots, nilOffset);

		std::unordered_set<uintptr_t> seenTree;
		std::unordered_set<uintptr_t> seenHeap;
		auto lastClean = std::chrono::steady_clock::now();
		auto lastRetry = std::chrono::steady_clock::now();
		auto lastHeapScan = std::chrono::steady_clock::now();

		while (m_running.load()) {
			DWORD exitCode = 0;
			if (!GetExitCodeProcess(hProc, &exitCode) || exitCode != STILL_ACTIVE)
				break;

			auto now = std::chrono::steady_clock::now();

			if (foundTrees) {
				for (auto rootOff : roots) {
					uintptr_t sentinel = 0;
					SIZE_T n = 0;
					if (!ReadProcessMemory(hProc, (LPCVOID)(dllBase + rootOff), &sentinel, 8, &n) || !IsValid(sentinel))
						continue;
					TraverseTree(hProc, sentinel, nilOffset, seenTree, allMods);
				}
			}

			auto heapElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeapScan).count();
			if (heapElapsed >= 3) {
				HeapScan(hProc, allMods, seenHeap);
				lastHeapScan = now;
			}

			if (!foundTrees) {
				auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRetry).count();
				if (elapsed >= 15) {
					foundTrees = ScanDataSection(hProc, dllBase, dllSize, allMods, roots, nilOffset);
					lastRetry = now;
				}
			}

			if (std::chrono::duration_cast<std::chrono::seconds>(now - lastClean).count() > 15) {
				seenTree.clear();
				seenHeap.clear();
				lastClean = now;
			}

			Sleep(5);
		}

		CloseHandle(hProc);
		m_running.store(false);
	}
};

inline cEventScanner g_EventScanner;

}
}
