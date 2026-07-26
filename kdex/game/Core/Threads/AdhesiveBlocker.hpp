#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <set>
#include <algorithm>
#include <Includes/Includes.hpp>
#include <Security/xorstr.hpp>

#pragma comment(lib, "psapi.lib")

namespace Core
{
	namespace Threads
	{
		class cAdhesiveBlocker
		{
		private:
			typedef NTSTATUS(NTAPI* fnNtQueryInformationThread)(
				HANDLE, ULONG, PVOID, ULONG, PULONG);

			struct ThreadProfile {
				DWORD id;
				uintptr_t start;
				unsigned __int64 userTime;
			};

			std::set<DWORD> ThrottledIds;
			fnNtQueryInformationThread NtQueryInfoThread = nullptr;
			uintptr_t CachedAdhesiveBase = 0;
			size_t CachedAdhesiveSize = 0;

			uintptr_t FindAdhesiveBase(HANDLE hProcess, size_t& modSize) {
				if (CachedAdhesiveBase) {
					modSize = CachedAdhesiveSize;
					return CachedAdhesiveBase;
				}

				HMODULE hMods[1024];
				DWORD cbNeeded;
				if (!EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
					return 0;

				for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
					char szModName[MAX_PATH];
					if (!GetModuleBaseNameA(hProcess, hMods[i], szModName, sizeof(szModName)))
						continue;
					std::string name = szModName;
					for (auto& c : name) c = (char)tolower(c);
					if (name.find(xorstr("adhesive")) == std::string::npos)
						continue;
					MODULEINFO modInfo;
					GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo));
					modSize = modInfo.SizeOfImage;
					CachedAdhesiveBase = (uintptr_t)modInfo.lpBaseOfDll;
					CachedAdhesiveSize = modSize;
					return CachedAdhesiveBase;
				}
				return 0;
			}

			bool SuspendAdhesiveThreads(DWORD pid) {
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
				if (!hProcess) return false;

				size_t adhesiveSize = 0;
				uintptr_t adhesiveBase = FindAdhesiveBase(hProcess, adhesiveSize);
				if (!adhesiveBase) {
					CloseHandle(hProcess);
					return false;
				}

				if (!NtQueryInfoThread) {
					NtQueryInfoThread = (fnNtQueryInformationThread)GetProcAddress(
						GetModuleHandleA(xorstr("ntdll.dll")),
						xorstr("NtQueryInformationThread"));
					if (!NtQueryInfoThread) {
						CloseHandle(hProcess);
						return false;
					}
				}

				std::vector<ThreadProfile> candidates;
				HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
				if (hSnap == INVALID_HANDLE_VALUE) {
					CloseHandle(hProcess);
					return false;
				}

				THREADENTRY32 te;
				te.dwSize = sizeof(te);
				int alreadyCount = 0;
				int totalAdhesive = 0;
				if (Thread32First(hSnap, &te)) {
					do {
						if (te.th32OwnerProcessID != pid) continue;
						HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
						if (!hThread) continue;
						uintptr_t startAddr = 0;
						if (NtQueryInfoThread(hThread, 9, &startAddr, sizeof(startAddr), NULL) == 0) {
							if (startAddr >= adhesiveBase && startAddr < adhesiveBase + adhesiveSize) {
								totalAdhesive++;
								if (ThrottledIds.count(te.th32ThreadID)) {
									alreadyCount++;
								}
								else {
									FILETIME c, e, k, u;
									if (GetThreadTimes(hThread, &c, &e, &k, &u)) {
										ULARGE_INTEGER uli;
										uli.LowPart = u.dwLowDateTime;
										uli.HighPart = u.dwHighDateTime;
										candidates.push_back({ te.th32ThreadID, startAddr, uli.QuadPart });
									}
								}
							}
						}
						CloseHandle(hThread);
					} while (Thread32Next(hSnap, &te));
				}
				CloseHandle(hSnap);

				if (totalAdhesive > 0 && alreadyCount == totalAdhesive) {
					CloseHandle(hProcess);
					std::fprintf(stderr, xorstr("[Adhesive] already blocked (%d threads)\n"), alreadyCount);
					return true;
				}

				if (candidates.empty()) {
					CloseHandle(hProcess);
					return false;
				}

				std::sort(candidates.begin(), candidates.end(),
					[](const ThreadProfile& a, const ThreadProfile& b) {
						return a.userTime > b.userTime;
					});

				int throttled = 0;
				for (size_t i = 0; i < candidates.size() && i < 3; i++) {
					if (candidates[i].userTime <= 500000) continue;
					HANDLE hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, candidates[i].id);
					if (!hThread) continue;
					SetThreadPriority(hThread, THREAD_PRIORITY_IDLE);
					ThrottledIds.insert(candidates[i].id);
					throttled++;
					CloseHandle(hThread);
				}

				CloseHandle(hProcess);
				return throttled > 0;
			}

		public:
			void Update()
			{
				while (!g_Variables.g_Unload)
				{
					if (g_Variables.ProcIdFiveM != 0) {
						if (SuspendAdhesiveThreads(g_Variables.ProcIdFiveM)) {
							std::fprintf(stderr, xorstr("[Adhesive] blocked\n"));
							return;
						}
					}

					std::this_thread::sleep_for(std::chrono::seconds(3));
				}
			}
		};

		inline cAdhesiveBlocker g_AdhesiveBlocker;
	}
}
