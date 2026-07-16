#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <iostream>
#include <string>
#include "../game/Security/xorstr.hpp"
#include <utility>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <Psapi.h>
#include <TlHelp32.h>
#include "imgui.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Utils {
    inline int GenRandomInt(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    inline float GenRandomFloat(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }

    inline void OpenConsole() {
        AllocConsole();
        FILE* f;
        freopen_s(&f, xorstr("CONIN$"), xorstr("r"), stdin);
        freopen_s(&f, xorstr("CONOUT$"), xorstr("w"), stdout);
        freopen_s(&f, xorstr("CONOUT$"), xorstr("w"), stderr);
    }

    inline void MoveConsoleToBottomRight() {
        HWND hCon = GetConsoleWindow();
        if (!hCon) return;
        RECT workArea = { 0 };
        if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) return;
        RECT wndRect = { 0 };
        if (!GetWindowRect(hCon, &wndRect)) return;
        int w = wndRect.right - wndRect.left;
        int h = wndRect.bottom - wndRect.top;
        int x = workArea.right - w;
        int y = workArea.bottom - h;
        SetWindowPos(hCon, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    inline std::pair<ImVec2, ImVec2> GetWindowPosAndSize(HWND hwnd) {
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            ImVec2 pos = { (float)rect.left, (float)rect.top };
            ImVec2 size = { (float)(rect.right - rect.left), (float)(rect.bottom - rect.top) };
            return { pos, size };
        }
        return { {0,0}, {0,0} };
    }

    inline std::string StringToFirstUpperCase(const std::string& str) {
        if (str.empty()) return str;
        std::string result = str;
        result[0] = std::toupper(result[0]);
        return result;
    }

    inline bool KeyPressedWithDelay(int key, int delay_ms) {
        static std::unordered_map<int, ULONGLONG> key_timers;
        ULONGLONG current_time = GetTickCount64();

        if (GetAsyncKeyState(key) & 0x8000) {
            auto [it, inserted] = key_timers.emplace(key, current_time);
            if (inserted || current_time - it->second >= static_cast<ULONGLONG>(delay_ms)) {
                it->second = current_time;
                return true;
            }
        }
        return false;
    }

    inline std::string EncodeB64(const std::string& in) {
        const char* b64 = xorstr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
        std::string out;
        int val = 0, valb = -6;
        for (uint8_t c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(b64[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    inline std::string DecodeB64(const std::string& in) {
        static const int T[256] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
            52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
        };
        std::string out;
        int val = 0, valb = -8;
        for (uint8_t c : in) {
            if (T[c] == -1) {
                if (c == '=') break;
                continue;
            }
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    inline std::string Str2Hex(const std::string& in) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (unsigned char c : in) {
            oss << std::setw(2) << int(c);
        }
        return oss.str();
    }

    inline std::string IntToHex(uintptr_t val) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << val;
        return oss.str();
    }

    inline std::string Hex2Str(const std::string& hex) {
        std::string out;
        out.reserve(hex.size() / 2);
        auto nibble = [](char c) -> unsigned char {
            if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
            return 0;
        };
        for (size_t i = 0; i + 1 < hex.size(); i += 2)
            out.push_back(static_cast<char>((nibble(hex[i]) << 4) | nibble(hex[i + 1])));
        return out;
    }

    inline void PasteClipboard(const char* text) {
        if (!text) return;
        if (!OpenClipboard(nullptr)) return;
        EmptyClipboard();
        size_t len = strlen(text) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem) {
            void* ptr = GlobalLock(hMem);
            if (ptr) {
                memcpy(ptr, text, len);
                GlobalUnlock(hMem);
                SetClipboardData(CF_TEXT, hMem);
            } else {
                GlobalFree(hMem);
            }
        }
        CloseClipboard();
    }

    inline bool IsFiveMProcessName(const wchar_t* nameLower) {
        if (!nameLower || !*nameLower) return false;

        wchar_t kFivem[]     = L"fivem";
        wchar_t kGtaProc[]   = L"gtaprocess";
        wchar_t kGameProc[]  = L"gameprocess";
        wchar_t kFivemExe[]  = L"fivem.exe";

        if (wcscmp(nameLower, kFivemExe) == 0)
            return true;

        const wchar_t* hasFivem = wcsstr(nameLower, kFivem);
        if (!hasFivem)
            return false;

        if (wcsstr(nameLower, kGtaProc) || wcsstr(nameLower, kGameProc))
            return true;

        return false;
    }

    inline HWND FindFiveMWindow() {
         struct EnumData {
             DWORD pid = 0;
             HWND found = nullptr;
         };
         EnumData data;

         HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
         if (hSnapshot != INVALID_HANDLE_VALUE) {
             PROCESSENTRY32W pe32;
             pe32.dwSize = sizeof(pe32);
             if (Process32FirstW(hSnapshot, &pe32)) {
                 do {
                     wchar_t nameLower[MAX_PATH];
                     wcscpy_s(nameLower, pe32.szExeFile);
                     _wcslwr_s(nameLower, MAX_PATH);

                     if (IsFiveMProcessName(nameLower)) {
                         data.pid = pe32.th32ProcessID;
                         break;
                     }
                 } while (Process32NextW(hSnapshot, &pe32));
             }
             CloseHandle(hSnapshot);
         }

         if (data.pid != 0) {
             EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
                 EnumData* pData = (EnumData*)lp;
                 DWORD windowPid = 0;
                 GetWindowThreadProcessId(hwnd, &windowPid);
                 if (windowPid == pData->pid) {
                     char className[256] = { 0 };
                     if (GetClassNameA(hwnd, className, sizeof(className)) && strcmp(className, xorstr("grcWindow")) == 0) {
                         pData->found = hwnd;
                         return FALSE;
                     }
                 }
                 return TRUE;
             }, (LPARAM)&data);

             if (data.found) return data.found;
         }

         EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
             EnumData* pData = (EnumData*)lp;
             char className[256] = { 0 };
             if (GetClassNameA(hwnd, className, sizeof(className)) && strcmp(className, xorstr("grcWindow")) == 0) {
                 if (IsWindowVisible(hwnd)) {
                     char title[256] = { 0 };
                     GetWindowTextA(hwnd, title, sizeof(title));
                     std::string sTitle = title;
                     std::transform(sTitle.begin(), sTitle.end(), sTitle.begin(), ::tolower);
                     if (sTitle.find(xorstr("fivem")) != std::string::npos) {
                         pData->found = hwnd;
                         return FALSE;
                     }
                 }
             }
             return TRUE;
         }, (LPARAM)&data);

         return data.found;
     }
}
