#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <intrin.h>
#include <string>
#include <algorithm>
#include "../../xorstr.hpp"

namespace AntiCrack {
namespace AntiVM {

    inline std::wstring widen(const char* s) {
        if (!s || !*s) return std::wstring();
        int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (n <= 0) return std::wstring();
        std::wstring out;
        out.resize(static_cast<size_t>(n));
        MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], n);
        return out;
    }

    inline bool is_hypervisor_cpuid() {
        int regs[4];
        __cpuid(regs, 1);
        return (regs[2] & (1u << 31)) != 0;
    }

    inline bool is_known_hypervisor_vendor() {
        int regs[4];
        __cpuid(regs, 0x40000000);
        char vendor[13] = { 0 };
        *(int*)(vendor + 0) = regs[1];
        *(int*)(vendor + 4) = regs[2];
        *(int*)(vendor + 8) = regs[3];
        std::string v(vendor);
        std::transform(v.begin(), v.end(), v.begin(), ::tolower);
        if (v.find(xorstr("vmware")) != std::string::npos) return true;
        if (v.find(xorstr("vbox")) != std::string::npos) return true;
        if (v.find(xorstr("xen")) != std::string::npos) return true;
        if (v.find(xorstr("kvm")) != std::string::npos) return true;
        if (v.find(xorstr("qemu")) != std::string::npos) return true;
        if (v.find(xorstr("microsoft")) != std::string::npos && v.find(xorstr("hyper")) != std::string::npos) return true;
        if (v.find(xorstr("prl")) != std::string::npos) return true;
        return false;
    }

    inline bool has_vm_registry_keys() {
        HKEY hKey = nullptr;
        for (int i = 0; i < 13; i++) {
            std::wstring keyW;
            switch (i) {
                case 0:  keyW = widen(xorstr("SOFTWARE\\VMware, Inc.\\VMware Tools")); break;
                case 1:  keyW = widen(xorstr("SOFTWARE\\Oracle\\VirtualBox Guest Additions")); break;
                case 2:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\VBoxGuest")); break;
                case 3:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\VBoxMouse")); break;
                case 4:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\VBoxService")); break;
                case 5:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\VBoxSF")); break;
                case 6:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\vmci")); break;
                case 7:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\vmrawdsk")); break;
                case 8:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\vmmouse")); break;
                case 9:  keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\vmvss")); break;
                case 10: keyW = widen(xorstr("SYSTEM\\CurrentControlSet\\Services\\vmhgfs")); break;
                case 11: keyW = widen(xorstr("SOFTWARE\\Xen")); break;
                case 12: keyW = widen(xorstr("SOFTWARE\\QEMU")); break;
                default: return false;
            }
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyW.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }
        }
        return false;
    }

    inline bool has_vm_mac_prefix() {
        return false;
    }

    inline bool has_vm_system_strings() {
        HKEY hKey = nullptr;
        wchar_t value[256] = { 0 };
        DWORD size = sizeof(value);
        std::wstring biosKey = widen(xorstr("HARDWARE\\DESCRIPTION\\System\\BIOS"));
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, biosKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return false;
        bool vm = false;
        std::wstring valManufacturer = widen(xorstr("SystemManufacturer"));
        if (RegQueryValueExW(hKey, valManufacturer.c_str(), nullptr, nullptr, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            std::wstring s(value);
            std::transform(s.begin(), s.end(), s.begin(), ::towlower);
            if (s.find(widen(xorstr("vmware"))) != std::wstring::npos ||
                s.find(widen(xorstr("virtualbox"))) != std::wstring::npos ||
                s.find(widen(xorstr("innotek"))) != std::wstring::npos ||
                s.find(widen(xorstr("xen"))) != std::wstring::npos ||
                s.find(widen(xorstr("qemu"))) != std::wstring::npos ||
                s.find(widen(xorstr("bochs"))) != std::wstring::npos ||
                s.find(widen(xorstr("microsoft corporation"))) != std::wstring::npos)
                vm = true;
        }
        size = sizeof(value);
        memset(value, 0, sizeof(value));
        std::wstring valProduct = widen(xorstr("SystemProductName"));
        if (!vm && RegQueryValueExW(hKey, valProduct.c_str(), nullptr, nullptr, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            std::wstring s(value);
            std::transform(s.begin(), s.end(), s.begin(), ::towlower);
            if (s.find(widen(xorstr("virtual"))) != std::wstring::npos ||
                s.find(widen(xorstr("vmware"))) != std::wstring::npos ||
                s.find(widen(xorstr("vbox"))) != std::wstring::npos ||
                s.find(widen(xorstr("virtual machine"))) != std::wstring::npos ||
                s.find(widen(xorstr("xen"))) != std::wstring::npos ||
                s.find(widen(xorstr("qemu"))) != std::wstring::npos ||
                s.find(widen(xorstr("kvm"))) != std::wstring::npos ||
                s.find(widen(xorstr("hyper-v"))) != std::wstring::npos)
                vm = true;
        }
        RegCloseKey(hKey);
        return vm;
    }

    inline bool is_running_in_vm() {
        if (is_hypervisor_cpuid() && is_known_hypervisor_vendor()) return true;
        if (is_known_hypervisor_vendor()) return true;
        if (has_vm_registry_keys()) return true;
        if (has_vm_mac_prefix()) return true;
        if (has_vm_system_strings()) return true;
        return false;
    }

}
}
