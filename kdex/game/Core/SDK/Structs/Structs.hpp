#pragma once
#include <vector>
#include <iostream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <sstream>
#include <psapi.h>
#include <regex>
#include <string>
#pragma comment(lib, "ntdll.lib")

#ifndef NTSYSCALLAPI
#define NTSYSCALLAPI __declspec(dllimport)
#endif

extern "C" NTSYSCALLAPI NTSTATUS ZwReadVirtualMemory(
	HANDLE  hProcess,
	LPCVOID lpBaseAddress,
	LPVOID  lpBuffer,
	SIZE_T  nSize,
	SIZE_T * lpNumberOfBytesRead
);

extern "C" NTSYSCALLAPI NTSTATUS ZwWriteVirtualMemory(
	HANDLE  hProcess,
	LPVOID  lpBaseAddress,
	LPCVOID lpBuffer,
	SIZE_T  nSize,
	SIZE_T * lpNumberOfBytesWritten
);
