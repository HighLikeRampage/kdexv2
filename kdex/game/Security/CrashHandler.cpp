#include "CrashHandler.hpp"
#include "framework/Globals.hpp"
#include "Api/api.hpp"
#include "../Gui/Overlay/Overlay.hpp"
#include "../Core/Features/SilentAim.hpp"
#include "../Core/Features/MagicBullets.hpp"
#include "../Core/Features/Exploits/Exploits.hpp"
#include "../Core/Threads/AdhesiveBlocker.hpp"
#include "../Core/Streamproof/Streamproof.h"
#include "../../framework/settings/variables.h"
#include <atomic>
#include <exception>
#include <eh.h>

namespace CrashHandler {

    static std::atomic<bool> g_unload_done{ false };

    static void ClearLoggedStateSafe() {
        __try {
            g_MenuInfo.IsLogged = false;
            if (var && var->auth.authenticated && !var->auth.access_token.empty())
                Security::Api::fivem_set_logged(var->auth.access_token, false);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    static void PerformEmergencyUnloadImpl() {
        if (g_unload_done.exchange(true))
            return;

        __try {
            Gui::RequestEmergencyStop();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        ClearLoggedStateSafe();

        __try {
            Core::Features::g_SilentAim.RestoreSilent();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        __try {
            Core::Features::g_MagicBullets.Restore();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        __try {
            Core::Features::Exploits::RestoreAll();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        __try {
            Core::Threads::g_AdhesiveBlocker.Restore();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        __try {
            streamProof.Cleanup();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void PerformEmergencyUnload() {
        PerformEmergencyUnloadImpl();
    }

    static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExInfo) {
        __try {
            PerformEmergencyUnloadImpl();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static LONG WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS* pExInfo) {
        if (!pExInfo || pExInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
            return EXCEPTION_CONTINUE_SEARCH;

        switch (pExInfo->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_IN_PAGE_ERROR:
            __try {
                PerformEmergencyUnloadImpl();
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            return EXCEPTION_CONTINUE_SEARCH;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    static void TerminateHandler() {
        __try {
            PerformEmergencyUnloadImpl();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        std::abort();
    }

    void Install() {
        static std::atomic<bool> s_installed{ false };
        if (s_installed.exchange(true))
            return;
        SetUnhandledExceptionFilter(UnhandledExceptionFilter);
        AddVectoredExceptionHandler(1, VectoredExceptionHandler);
        std::set_terminate(TerminateHandler);
    }

}
