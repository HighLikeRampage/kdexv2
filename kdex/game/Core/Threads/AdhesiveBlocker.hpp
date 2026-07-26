#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
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

			bool Done = false;

			uintptr_t FindAdhesiveBase(HANDLE hProcess, size_t& modSize) {
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
					return (uintptr_t)modInfo.lpBaseOfDll;
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

				fnNtQueryInformationThread NtQueryInfoThread =
					(fnNtQueryInformationThread)GetProcAddress(
						GetModuleHandleA(xorstr("ntdll.dll")),
						xorstr("NtQueryInformationThread"));
				if (!NtQueryInfoThread) {
					CloseHandle(hProcess);
					return false;
				}

				std::vector<ThreadProfile> candidates;
				HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
				if (hSnap == INVALID_HANDLE_VALUE) {
					CloseHandle(hProcess);
					return false;
				}

				THREADENTRY32 te;
				te.dwSize = sizeof(te);
				if (Thread32First(hSnap, &te)) {
					do {
						if (te.th32OwnerProcessID != pid) continue;
						HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
						if (!hThread) continue;
						uintptr_t startAddr = 0;
						if (NtQueryInfoThread(hThread, 9, &startAddr, sizeof(startAddr), NULL) == 0) {
							if (startAddr >= adhesiveBase && startAddr < adhesiveBase + adhesiveSize) {
								FILETIME c, e, k, u;
								if (GetThreadTimes(hThread, &c, &e, &k, &u)) {
									ULARGE_INTEGER uli;
									uli.LowPart = u.dwLowDateTime;
									uli.HighPart = u.dwHighDateTime;
									candidates.push_back({ te.th32ThreadID, startAddr, uli.QuadPart });
								}
							}
						}
						CloseHandle(hThread);
					} while (Thread32Next(hSnap, &te));
				}
				CloseHandle(hSnap);

				if (candidates.empty()) {
					CloseHandle(hProcess);
					return false;
				}

				std::sort(candidates.begin(), candidates.end(),
					[](const ThreadProfile& a, const ThreadProfile& b) {
						return a.userTime > b.userTime;
					});

				int suspended = 0;
				for (size_t i = 0; i < candidates.size() && i < 3; i++) {
					if (candidates[i].userTime <= 500000) continue;
					HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, candidates[i].id);
					if (!hThread) continue;
					SuspendThread(hThread);
					suspended++;
					CloseHandle(hThread);
				}

				CloseHandle(hProcess);
				return suspended > 0;
			}

		public:
			void Update()
			{
				while (!g_Variables.g_Unload)
				{
					if (Done) {
						std::this_thread::sleep_for(std::chrono::seconds(5));
						continue;
					}

					if (g_Variables.ProcIdFiveM != 0) {
						if (SuspendAdhesiveThreads(g_Variables.ProcIdFiveM)) {
							Done = true;
							std::fprintf(stderr, xorstr("[Adhesive] blocked\n"));
						}
					}

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
			}
		};

		inline cAdhesiveBlocker g_AdhesiveBlocker;
	}
}
