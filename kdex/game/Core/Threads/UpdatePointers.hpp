#pragma once
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>
#include <string>

namespace Core
{
    namespace Threads
    {
        class cUpdatePtrs {
        private:
            static void DoUpdate()
            {
                __try {
                    Core::SDK::Pointers::pWorld = Mem.Read<CPedFactory *>( g_Offsets.m_World );
                    Core::SDK::Pointers::pLocalPlayer = Core::SDK::Pointers::pWorld
                        ? Core::SDK::Pointers::pWorld->GetLocalPlayer()
                        : nullptr;
                    Core::SDK::Pointers::pReplayInterFace = Mem.Read<CReplayInterFace *>( g_Offsets.m_ReplayInterFace );
                    Core::SDK::Pointers::pViewPort = Mem.Read<uintptr_t>( g_Offsets.m_ViewPort );
                    Core::SDK::Pointers::pCamGamePlayDirector = Mem.Read<uintptr_t>( g_Offsets.m_CamGameplayDirector );
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        public:
            void Update( )
            {
                while ( !g_Variables.g_Unload )
                {
                    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
                    if (!Core::g_AttachedToGame) continue;
                    DoUpdate();
                }
            }
        };

        inline cUpdatePtrs g_UpdatePtrs;
    }
}
