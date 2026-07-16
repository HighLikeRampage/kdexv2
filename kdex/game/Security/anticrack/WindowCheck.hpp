#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <Psapi.h>
#include "../../xorstr.hpp"

#pragma comment(lib, "psapi.lib")

namespace AntiCrack {
namespace WindowCheck {

    inline std::wstring widen(const char* s) {
        if (!s || !*s) return std::wstring();
        int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (n <= 0) return std::wstring();
        std::wstring out;
        out.resize(static_cast<size_t>(n));
        MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], n);
        return out;
    }

    inline std::wstring expected_overlay_title() {
        return widen(xorstr("Rotten Overlay"));
    }

    inline bool is_our_window_title_valid(HWND our_hwnd) {
        if (!our_hwnd || !IsWindow(our_hwnd)) return true;
        wchar_t buf[256] = { 0 };
        if (GetWindowTextW(our_hwnd, buf, _countof(buf)) <= 0) return true;
        return wcscmp(buf, expected_overlay_title().c_str()) == 0;
    }

    inline const std::vector<std::string>& bad_titles() {
        static const std::vector<std::string> list = {
            xorstr("httpdebugger"), xorstr("http debugger"), xorstr("x64dbg"), xorstr("debugger"),
            xorstr("disassembler"), xorstr("decompiler"), xorstr("fiddler"), xorstr("wireshark"),
            xorstr("string search"), xorstr("process list"), xorstr("memory viewer"), xorstr("system informer"),
            xorstr("process hacker"), xorstr("ghidra"), xorstr("binary ninja"), xorstr("hyperdbg"),
            xorstr("process explorer"), xorstr("extreme dumper"), xorstr("scylla"), xorstr("windbg"),
            xorstr("ksdumper"), xorstr("ollydbg"), xorstr("petool"), xorstr("beamer"),
            xorstr("analysis tool"), xorstr("referenced strings"), xorstr("dissect code"),
            xorstr("import reconstructor"), xorstr("httpdebuggerui"), xorstr("add address"),
            xorstr("process telerik"), xorstr("network traffic dump"), xorstr("wireshark packet"),
            xorstr("part of sysinternals"), xorstr("network analyzer"), xorstr("[elevated]"),
            xorstr("codecave hook"), xorstr("ida"), xorstr("immunity"),
        };
        return list;
    }

    inline std::string get_process_name(DWORD pid) {
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!h) return std::string(xorstr(""));
        char path[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (!QueryFullProcessImageNameA(h, 0, path, &size)) {
            CloseHandle(h);
            return std::string(xorstr(""));
        }
        CloseHandle(h);
        const char* name = strrchr(path, '\\');
        if (name) name++; else name = path;
        std::string s(name);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    inline bool is_blacklisted_process(const std::string& name) {
        static const std::vector<std::string> blacklist = {
            xorstr("x64dbg.exe"), xorstr("x32dbg.exe"), xorstr("ollydbg.exe"), xorstr("ida.exe"),
            xorstr("ida64.exe"), xorstr("idag.exe"), xorstr("idag64.exe"), xorstr("windbg.exe"),
            xorstr("dnspy.exe"), xorstr("de4dot.exe"), xorstr("processhacker.exe"), xorstr("procmon.exe"),
            xorstr("procexp.exe"), xorstr("procexp64.exe"), xorstr("fiddler.exe"), xorstr("wireshark.exe"),
            xorstr("httpdebuggerpro.exe"), xorstr("ksdumper.exe"), xorstr("scylla.exe"), xorstr("petool.exe"),
            xorstr("importrec.exe"), xorstr("reshacker.exe"), xorstr("cheatengine-x86_64.exe"),
            xorstr("cheatengine-i386.exe"), xorstr("ghidra"), xorstr("binaryninja"), xorstr("x64netdumper.exe"),
            xorstr("extremedumper.exe"), xorstr("dumpcap.exe"), xorstr("hookanalyzer"), xorstr("httpanalyzer"),
        };
        for (const auto& b : blacklist)
            if (name.find(b) != std::string::npos) return true;
        return false;
    }

    inline BOOL CALLBACK enum_windows_callback(HWND hWnd, LPARAM lParam) {
        int len = GetWindowTextLengthA(hWnd);
        if (len <= 0 || !IsWindowVisible(hWnd)) return TRUE;
        std::vector<char> buf(len + 1, 0);
        GetWindowTextA(hWnd, buf.data(), len + 1);
        std::string title(buf.data());
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        for (const auto& s : bad_titles())
            if (title.find(s) != std::string::npos) return FALSE;
        return TRUE;
    }

    inline bool no_bad_windows() {
        return EnumWindows(enum_windows_callback, 0) != FALSE;
    }

    inline bool no_blacklisted_processes() {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return true;
        PROCESSENTRY32 pe = { sizeof(pe) };
        if (!Process32First(snap, &pe)) { CloseHandle(snap); return true; }
        do {
            if (pe.th32ProcessID == GetCurrentProcessId()) continue;
            std::string name = get_process_name(pe.th32ProcessID);
            if (!name.empty() && is_blacklisted_process(name)) {
                CloseHandle(snap);
                return false;
            }
        } while (Process32Next(snap, &pe));
        CloseHandle(snap);
        return true;
    }

}
}
