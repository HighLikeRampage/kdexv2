#pragma once
#include <thread>
#include <string>
#include <cstddef>
#include <Psapi.h>
#include "../../xorstr.hpp"
#include <iomanip>
#include <iostream>
#include <Shlwapi.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>
#include <handleapi.h>

#pragma comment( lib, "ntdll.lib"  )
#pragma comment( lib, "shlwapi.lib" )
#pragma comment( lib, "Kernel32.lib" )
#pragma comment( lib, "psapi.lib" )

namespace AntiCrack
{

#pragma region nt_struct

#define NT_SUCCESS( x ) ( ( x ) >= 0 )
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

#define SystemHandleInformation 16
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PWCH   Buffer;
	} UNICODE_STRING, * PUNICODE_STRING;

	typedef struct _OBJECT_ATTRIBUTES {
		ULONG           Length;
		HANDLE          RootDirectory;
		PUNICODE_STRING ObjectName;
		ULONG           Attributes;
		PVOID           SecurityDescriptor;
		PVOID           SecurityQualityOfService;
	}  OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

	typedef struct _CLIENT_ID {
		PVOID UniqueProcess;
		PVOID UniqueThread;
	} CLIENT_ID, * PCLIENT_ID;

	typedef NTSTATUS( NTAPI * _NtQuerySystemInformation )(
		ULONG SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
		);

	typedef NTSTATUS( NTAPI * _NtDuplicateObject )(
		HANDLE SourceProcessHandle,
		HANDLE SourceHandle,
		HANDLE TargetProcessHandle,
		PHANDLE TargetHandle,
		ACCESS_MASK DesiredAccess,
		ULONG Attributes,
		ULONG Options
		);

	typedef NTSYSAPI NTSTATUS( NTAPI * _NtOpenProcess )(
		PHANDLE            ProcessHandle,
		ACCESS_MASK        DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		PCLIENT_ID         ClientId
		);

	typedef NTSTATUS( NTAPI * _NtQueryObject )(
		HANDLE ObjectHandle,
		ULONG ObjectInformationClass,
		PVOID ObjectInformation,
		ULONG ObjectInformationLength,
		PULONG ReturnLength
		);

	typedef NTSTATUS( NTAPI * _NtQueryVirtualMemory )(
		HANDLE ProcessHandle,
		PVOID BaseAddress,
		int MemoryInformationClass,
		PVOID MemoryInformation,
		SIZE_T MemoryInformationLength,
		PSIZE_T ReturnLength
		);

	typedef struct _SYSTEM_HANDLE {
		ULONG ProcessId;
		BYTE ObjectTypeNumber;
		BYTE Flags;
		USHORT Handle;
		PVOID Object;
		ACCESS_MASK GrantedAccess;
	} SYSTEM_HANDLE, * PSYSTEM_HANDLE;

	typedef struct _SYSTEM_HANDLE_INFORMATION {
		ULONG HandleCount;
		SYSTEM_HANDLE Handles[ 1 ];
	} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

	typedef enum _POOL_TYPE {
		NonPagedPool,
		PagedPool,
		NonPagedPoolMustSucceed,
		DontUseThisType,
		NonPagedPoolCacheAligned,
		PagedPoolCacheAligned,
		NonPagedPoolCacheAlignedMustS
	} POOL_TYPE, * PPOOL_TYPE;

	typedef struct _OBJECT_TYPE_INFORMATION {
		UNICODE_STRING Name;
		ULONG TotalNumberOfObjects;
		ULONG TotalNumberOfHandles;
		ULONG TotalPagedPoolUsage;
		ULONG TotalNonPagedPoolUsage;
		ULONG TotalNamePoolUsage;
		ULONG TotalHandleTableUsage;
		ULONG HighWaterNumberOfObjects;
		ULONG HighWaterNumberOfHandles;
		ULONG HighWaterPagedPoolUsage;
		ULONG HighWaterNonPagedPoolUsage;
		ULONG HighWaterNamePoolUsage;
		ULONG HighWaterHandleTableUsage;
		ULONG InvalidAttributes;
		GENERIC_MAPPING GenericMapping;
		ULONG ValidAccess;
		BOOLEAN SecurityRequired;
		BOOLEAN MaintainHandleCount;
		USHORT MaintainTypeList;
		POOL_TYPE PoolType;
		ULONG PagedPoolUsage;
		ULONG NonPagedPoolUsage;
	} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

#define NtCurrentProcess( ) (  ( HANDLE )( LONG_PTR ) -1  )

#pragma endregion

#define DbgBreakPoint_FUNC_SIZE 0x2
#define DbgUiRemoteBreakin_FUNC_SIZE 0x54
#define NtContinue_FUNC_SIZE 0x18

	struct FUNC {
		FARPROC addr;
		SIZE_T size;
	};

	inline const char* get_func_name(std::size_t i) {
		switch (i) {
			case 0: return xorstr("DbgBreakPoint");
			case 1: return xorstr("DbgUiRemoteBreakin");
			case 2: return xorstr("NtContinue");
			default: return xorstr("");
		}
	}

	static FUNC funcList[] = {
		{ 0, DbgBreakPoint_FUNC_SIZE },
		{ 0, DbgUiRemoteBreakin_FUNC_SIZE },
		{ 0, NtContinue_FUNC_SIZE }
	};
}
