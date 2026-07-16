#pragma once
#include <Includes/Includes.hpp>
#include <Security/Api/structs/structs.hpp>
#include <Security/Api/api.hpp>
#include <Core/SDK/Structs/Structs.hpp>
#include <Psapi.h>
#include <Security/anticrack/AntiVM.hpp>
#include <Security/anticrack/AntiTamper.hpp>
#include <Security/anticrack/AntiDebug.hpp>
#include <Security/anticrack/WindowCheck.hpp>
#include "xorstr.hpp"
#include <mutex>
#include <chrono>
#include <cstdlib>

#pragma comment(lib, "psapi.lib")

#define close_app ExitProcess( 0 );
#define force_crash abort( );

namespace AntiCrack {

	inline void ErasePEHeaderSafe( ) {
		return;
	}

	constexpr std::chrono::seconds kSecurityCheckIntervalSec{ 10 };
	constexpr ULONG kMaxHandleInfoSize = 4u * 1024u * 1024u;

	static std::string g_access_token_for_report;
	static std::mutex g_token_mutex;
	static HWND g_our_overlay_hwnd = nullptr;

	inline void SetAccessTokenForReport( const std::string& token ) {
		std::lock_guard<std::mutex> lock( g_token_mutex );
		g_access_token_for_report = token;
	}
	inline void SetOurOverlayHwnd( HWND hwnd ) { g_our_overlay_hwnd = hwnd; }

	inline void TriggerViolation( const char* reason, const char* details = "" ) {

		return;
	}

	inline BOOL CALLBACK enum_windows_callback( HWND hWnd, LPARAM lparam ) {
		int length = GetWindowTextLengthA( hWnd );
		if ( length <= 0 || length > 512 || !IsWindowVisible( hWnd ) ) return TRUE;
		static char buffer[ 513 ];
		buffer[ 0 ] = '\0';
		GetWindowTextA( hWnd, buffer, 513 );
		std::string title( buffer );
		std::transform( title.begin( ), title.end( ), title.begin( ), ::tolower );
		for ( const auto& s : WindowCheck::bad_titles() ) {
			if ( title.find( s ) != std::string::npos ) {
				TriggerViolation( xorstr( "window_tamper" ), xorstr( "suspicious_window" ) );
				return FALSE;
			}
		}
		return TRUE;
	}

	inline bool FindHookInAddr( BYTE * func ) {

		if ( func[ 0 ] == 0xE9 ) {
			return true;
		}

		if ( func[ 0 ] == 0xFF && func[ 1 ] == 0x25 ) {
			return true;
		}

		if ( func[ 0 ] == 0x90 && func[ 1 ] == 0x90 && func[ 2 ] == 0xE9 ) {
			return true;
		}

		return false;
	}

	inline void StopService( const char * service_name ) {

		SC_HANDLE service_manager = OpenSCManagerA( NULL, NULL, SC_MANAGER_ALL_ACCESS );
		if ( !service_manager ) { return; }

		SC_HANDLE service_handle = OpenServiceA( service_manager, service_name, SERVICE_QUERY_STATUS );

		if ( service_handle ) {
			SERVICE_STATUS serviceStatus;
			if ( QueryServiceStatus( service_handle, &serviceStatus ) && serviceStatus.dwCurrentState != SERVICE_STOPPED ) {
				SC_HANDLE stopServiceHandle = OpenServiceA( service_manager, service_name, SERVICE_STOP );
				if ( stopServiceHandle ) {
					SERVICE_STATUS stopServiceStatus = {};
					ControlService( stopServiceHandle, SERVICE_CONTROL_STOP, &stopServiceStatus );
					CloseServiceHandle( stopServiceHandle );
				}
			}
			CloseServiceHandle( service_handle );
		}

		CloseServiceHandle( service_manager );
	}

	namespace {
		static constexpr uintptr_t _k1 = 0x8A2F4C91B3E6D570ULL;
		static constexpr uintptr_t _k2 = 0xF4691E8A2D5B0C30ULL;
		static constexpr uintptr_t _k3 = 0x9E3779B97F4A7C15ULL;
		static constexpr uintptr_t _key( size_t i ) noexcept { return ( _k1 ^ _k2 ) + ( _k3 * i ); }
	}
	static const uintptr_t kApiFunctionsEnc[] = {
		( uintptr_t ) &FindHookInAddr ^ _key( 0 ),
		( uintptr_t ) &ReadProcessMemory ^ _key( 1 ),
		( uintptr_t ) &VirtualProtect ^ _key( 2 ),
		( uintptr_t ) &WriteProcessMemory ^ _key( 3 ),
		( uintptr_t ) &IsDebuggerPresent ^ _key( 4 ),
		( uintptr_t ) &GetTickCount64 ^ _key( 5 ),
		( uintptr_t ) &GetTickCount ^ _key( 6 ),
		( uintptr_t ) &FindWindowA ^ _key( 7 ),
		( uintptr_t ) &OpenProcess ^ _key( 8 ),
		( uintptr_t ) &exit ^ _key( 9 ),
		( uintptr_t ) &enum_windows_callback ^ _key( 10 ),
		( uintptr_t ) &CreateRemoteThread ^ _key( 11 ),
		( uintptr_t ) &CreateRemoteThreadEx ^ _key( 12 ),
		( uintptr_t ) &VirtualAllocEx ^ _key( 13 ),
		( uintptr_t ) &ZwReadVirtualMemory ^ _key( 14 ),
		( uintptr_t ) &ZwWriteVirtualMemory ^ _key( 15 ),
		( uintptr_t ) &FindWindowW ^ _key( 16 ),
		( uintptr_t ) &CheckRemoteDebuggerPresent ^ _key( 17 ),
		( uintptr_t ) &LoadLibraryA ^ _key( 18 ),
		( uintptr_t ) &SetWindowsHook ^ _key( 19 ),
		( uintptr_t ) &VirtualAlloc ^ _key( 20 ),
		( uintptr_t ) &CreateThread ^ _key( 21 ),
		( uintptr_t ) &OpenThread ^ _key( 22 ),
		( uintptr_t ) &ExitProcess ^ _key( 23 ),
		( uintptr_t ) &TerminateProcess ^ _key( 24 ),
	};
	static constexpr size_t kApiFunctionsCount = sizeof( kApiFunctionsEnc ) / sizeof( kApiFunctionsEnc[ 0 ] );
	inline const BYTE* GetApiFunction( size_t i ) noexcept {
		uintptr_t a = _k1 ^ _k2;
		uintptr_t b = _k3 * i;
		return ( const BYTE* )( kApiFunctionsEnc[ i ] ^ ( a + b ) );
	}

	inline void RunCheckWindows( ) {
		if ( !EnumWindows( enum_windows_callback, NULL ) )
			;
		if ( !WindowCheck::no_blacklisted_processes() )
			TriggerViolation( xorstr( "window_tamper" ), xorstr( "blacklisted_process" ) );
		if ( g_our_overlay_hwnd && !WindowCheck::is_our_window_title_valid( g_our_overlay_hwnd ) )
			TriggerViolation( xorstr( "window_tamper" ), xorstr( "overlay_title_changed" ) );
	}

	inline void RunCheckServices( ) {
		StopService( xorstr( "KSystemInformer" ) );
		StopService( xorstr( "KProcessHacker3" ) );
		StopService( xorstr( "HTTPDebuggerPro" ) );
		StopService( xorstr( "HttpDebuggerSdk" ) );
		StopService( xorstr( "KsDumper" ) );
		StopService( xorstr( "kdstinker" ) );
		StopService( xorstr( "NiGgEr" ) );
		StopService( xorstr( "iqvw64e" ) );
		StopService( xorstr( "AsUpIO64" ) );
		StopService( xorstr( "BS_Flash64" ) );
		StopService( xorstr( "Phymemx64" ) );
	}

	inline void RunCheckHookedFunc( ) {
		for ( size_t i = 0; i < kApiFunctionsCount; ++i ) {
			if ( FindHookInAddr( const_cast<BYTE*>( GetApiFunction( i ) ) ) )
				TriggerViolation( xorstr( "tamper" ), xorstr( "hooked_api" ) );
		}
	}

	inline void RunMemoryReadDetection( ) {
		static void* guard_page = nullptr;
		static _NtQueryVirtualMemory query_virtual_memory = nullptr;
		if ( !guard_page ) {
			guard_page = VirtualAlloc( nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
			HMODULE hNtdll = GetModuleHandleA( xorstr( "ntdll.dll" ) );
			query_virtual_memory = hNtdll ? ( _NtQueryVirtualMemory ) GetProcAddress( hNtdll, xorstr( "NtQueryVirtualMemory" ) ) : nullptr;
		}
		if ( !guard_page || !query_virtual_memory ) return;
		PSAPI_WORKING_SET_EX_INFORMATION info = { 0 };
		info.VirtualAddress = guard_page;
		query_virtual_memory( NtCurrentProcess( ), NULL, 4, &info, sizeof info, NULL );
		if ( info.VirtualAttributes.Valid )
			TriggerViolation( xorstr( "dump" ), xorstr( "memory_read" ) );
	}

	inline void RunAntiAttach( ) {
		DWORD pid = GetCurrentProcessId( );
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, 0, pid );
		if ( !hProcess ) return;
		HMODULE hMod = GetModuleHandleA( xorstr( "ntdll.dll" ) );
		if ( !hMod ) { CloseHandle( hProcess ); return; }
		for ( int i = 0; i < _countof( funcList ); ++i )
			funcList[ i ].addr = GetProcAddress( hMod, get_func_name( i ) );
		void* base_address = GetModuleHandleA( nullptr );
		if ( base_address && ( wcsstr( ( WCHAR* ) base_address, WindowCheck::widen( xorstr( "ntdll" ) ).c_str( ) ) || wcsstr( ( WCHAR* ) base_address, WindowCheck::widen( xorstr( "NTDLL" ) ).c_str( ) ) ) ) {
			for ( int i = 0; i < _countof( funcList ); ++i ) {
				if ( !funcList[ i ].addr ) continue;
				DWORD dwOldProtect;
				VirtualProtectEx( hProcess, funcList[ i ].addr, ( SIZE_T ) funcList[ i ].size, PAGE_EXECUTE_READWRITE, &dwOldProtect );
				WriteProcessMemory( hProcess, funcList[ i ].addr, funcList[ i ].addr, funcList[ i ].size, NULL );
				VirtualProtectEx( hProcess, funcList[ i ].addr, ( SIZE_T ) funcList[ i ].size, dwOldProtect, NULL );
			}
		}
		CloseHandle( hProcess );
	}

	inline OBJECT_ATTRIBUTES InitObjectAttributes( PUNICODE_STRING name, ULONG attributes, HANDLE hRoot, PSECURITY_DESCRIPTOR security )
	{
		OBJECT_ATTRIBUTES object;

		object.Length = sizeof( OBJECT_ATTRIBUTES );
		object.ObjectName = name;
		object.Attributes = attributes;
		object.RootDirectory = hRoot;
		object.SecurityDescriptor = security;

		return object;
	}

	inline HANDLE DumpHandle( HANDLE hProcessId, HANDLE hHandleValue ) {
		const auto NtOpenProcess = ( _NtOpenProcess ) GetProcAddress( GetModuleHandleA( xorstr( "ntdll.dll" ) ), xorstr( "NtOpenProcess" ) );

		HANDLE hRet = nullptr;
		NTSTATUS nsProcess;
		HANDLE hProcess;
		CLIENT_ID ProcessId = { 0 };
		ProcessId.UniqueProcess = hProcessId;

		OBJECT_ATTRIBUTES ObjectAttributes = InitObjectAttributes( NULL, NULL, NULL, NULL );

		nsProcess = NtOpenProcess( &hProcess, PROCESS_ALL_ACCESS, &ObjectAttributes, &ProcessId );

		if ( NT_SUCCESS( nsProcess ) ) {
			std::string systemDrive = getenv( xorstr( "SystemDrive" ) );
			std::vector<std::string> systemFiles = {
				xorstr( "\\Windows\\System32\\svchost.exe" ),
				xorstr( "\\Windows\\System32\\conhost.exe" ),
				xorstr( "\\Windows\\System32\\lsass.exe" ),
				xorstr( "\\Windows\\explorer.exe" ),
				xorstr( "\\Windows\\System32\\csrss.exe" ),
				xorstr( "\\Windows\\System32\\wininit.exe" ),
				xorstr( "\\Windows\\System32\\winlogon.exe" )
			};

			char badPath[ MAX_PATH ];
			GetModuleFileNameExA( hProcess, nullptr, badPath, MAX_PATH );

			bool is_system_file = false;
			for ( const auto & file : systemFiles ) {
				if ( !strcmp( badPath, ( systemDrive + file ).c_str( ) ) ) {
					is_system_file = true;
					break;
				}
			}

			if ( is_system_file ) {
				CloseHandle( hProcess );
				return nullptr;
			}

			NTSTATUS nsDup;
			HANDLE hLocalHandle;
			nsDup = DuplicateHandle( hProcess, hHandleValue, GetCurrentProcess( ), &hLocalHandle, 0L, 0L, DUPLICATE_SAME_ACCESS );
			if ( NT_SUCCESS( nsDup ) ) {
				hRet = hLocalHandle;
			}
			CloseHandle( hProcess );
		}

		return hRet;
	}

	inline void KillHandle( HANDLE hProcessId, HANDLE hHandleValue )
	{
		const auto NtOpenProcess = ( _NtOpenProcess ) GetProcAddress( GetModuleHandleA( xorstr( "ntdll.dll" ) ), xorstr( "NtOpenProcess" ) );

		if ( GetCurrentProcessId( ) == ( DWORD ) hProcessId )
		{
			return;
		}

		NTSTATUS nsProcess;
		HANDLE hProcess;
		CLIENT_ID ProcessId = { 0 };
		ProcessId.UniqueProcess = hProcessId;

		OBJECT_ATTRIBUTES ObjectAttributes = InitObjectAttributes( NULL, NULL, NULL, NULL );

		nsProcess = NtOpenProcess( &hProcess, PROCESS_ALL_ACCESS, &ObjectAttributes, &ProcessId );
		if ( NT_SUCCESS( nsProcess ) )
		{
			NTSTATUS nsDup;
			HANDLE hLocalHandle;
			nsDup = DuplicateHandle( hProcess, hHandleValue, GetCurrentProcess( ), &hLocalHandle, 0L, 0L, DUPLICATE_CLOSE_SOURCE );
			if ( NT_SUCCESS( nsDup ) )
			{
				CloseHandle( hLocalHandle );
			}
			CloseHandle( hProcess );
		}

	}

	inline void RunCheckProcesses( ) {
		const auto NtQuerySystemInformation = ( _NtQuerySystemInformation ) GetProcAddress( GetModuleHandleA( xorstr( "ntdll.dll" ) ), xorstr( "NtQuerySystemInformation" ) );
		if ( !NtQuerySystemInformation ) return;
		ULONG handleInfoSize = 0x10000;
		PSYSTEM_HANDLE_INFORMATION handleInfo = ( PSYSTEM_HANDLE_INFORMATION ) std::malloc( handleInfoSize );
		if ( !handleInfo ) return;
		NTSTATUS status;
		for ( ;; ) {
			status = NtQuerySystemInformation( SystemHandleInformation, handleInfo, handleInfoSize, NULL );
			if ( NT_SUCCESS( status ) ) break;
			if ( status != STATUS_INFO_LENGTH_MISMATCH || handleInfoSize >= kMaxHandleInfoSize ) { std::free( handleInfo ); return; }
			void* next = std::realloc( handleInfo, handleInfoSize *= 2 );
			if ( !next ) { std::free( handleInfo ); return; }
			handleInfo = ( PSYSTEM_HANDLE_INFORMATION ) next;
		}
		for ( ULONG i = 0; i < handleInfo->HandleCount; i++ ) {
			if ( handleInfo->Handles[ i ].ProcessId == 4 || handleInfo->Handles[ i ].ProcessId == GetCurrentProcessId( ) || ( int ) handleInfo->Handles[ i ].ObjectTypeNumber != 7 )
				continue;
			HANDLE hLocalHandle = DumpHandle( ( HANDLE ) handleInfo->Handles[ i ].ProcessId, ( HANDLE ) handleInfo->Handles[ i ].Handle );
			if ( hLocalHandle ) {
				if ( GetProcessId( hLocalHandle ) == GetCurrentProcessId( ) ) {
					KillHandle( ( HANDLE ) handleInfo->Handles[ i ].ProcessId, ( HANDLE ) handleInfo->Handles[ i ].Handle );
				}
				CloseHandle( hLocalHandle );
			}
		}
		std::free( handleInfo );
	}

	inline void RunAllSecurityChecks( ) {
		return;
	}

	inline void SecurityThreadFunc( ) {
		return;
	}

	inline std::string GetProcesssNameByPid( DWORD pid ) {
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid );

		if ( hProcess != NULL ) {
			char processName[ MAX_PATH ];
			if ( GetModuleFileNameExA( hProcess, NULL, processName, MAX_PATH ) ) {
				CloseHandle( hProcess );
				return processName;
			}
			else {

			}

			CloseHandle( hProcess );
		}
		else {

		}

		return xorstr( "" );
	}

	inline void DoProtect( ) {
		return;
	}
}
