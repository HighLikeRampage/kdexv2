#pragma once
// NativeSafeInit.hpp
// Drop-in patch for NativeCaller::Initialize() that skips Citizen mode.
//
// WHY: Citizen mode permanently overwrites a slot in citizen-scripting-core's
// native registration table with a pointer to our RWX shellcode.
// Adhesive scans that table for integrity violations.  Skipping Citizen mode
// avoids that entire detection surface.  APC and Direct modes never modify the
// native table and have a much smaller steady-state footprint.
//
// HOW TO USE:
//   In NativeCaller.cpp, replace the body of CNativeCaller::Initialize() with:
//
//       void CNativeCaller::Initialize() {
//           SafeInitialize();
//       }
//
//   Or call SafeInitialize() from your init path directly.

#include <Core/SDK/Natives/NativeCaller.hpp>

namespace NativeCaller {

    inline void CNativeCaller::SafeInitialize() {
        if (m_ready) return;
        if (!Mem.ProcId || !Mem.ProcHandle) {
            DebugLog(xorstr("[NC-safe] process not open\n"));
            return;
        }
        if (!m_build) m_build = DetectBuild();

        RefreshModRanges();

        // Scan citizen table for handler lookups even though we won't hook it.
        // APC and Direct modes still need citizen table to resolve native handlers.
        ScanCitizenTable();

        // Order: skip Citizen (table hook), try MainFn (ScriptHookV), APC, Direct.
        // In practice on most FiveM servers: MainFn is absent, APC succeeds.
        if (TryMainFnMode()) {
            m_ready = true;
            DebugLog(xorstr("[NC-safe] mode=mainfn build=%d\n"), m_build);
            return;
        }
        if (TryApcMode()) {
            m_ready = true;
            DebugLog(xorstr("[NC-safe] mode=apc build=%d\n"), m_build);
            return;
        }
        if (TryDirectMode()) {
            m_ready = true;
            DebugLog(xorstr("[NC-safe] mode=direct build=%d\n"), m_build);
            return;
        }
        // Last resort: citizen mode. Only reached if APC and Direct both fail.
        if (TryCitizenMode()) {
            m_ready = true;
            DebugLog(xorstr("[NC-safe] mode=citizen (fallback) build=%d\n"), m_build);
            return;
        }
        DebugLog(xorstr("[NC-safe] all modes failed\n"));
    }

}
