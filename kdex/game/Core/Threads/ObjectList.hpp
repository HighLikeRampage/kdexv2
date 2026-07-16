#pragma once
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>
#include <string>
#include <sstream>

namespace Core
{
    namespace Threads
    {
        class cObjectList {
        private:
            CObjectInterFace * ObjInterface = nullptr;
            CObjectList * ObjList = nullptr;
            int MaxObjects = 0;

            CPickupInterFace * PickupInterface = nullptr;
            CPickupList * PickList = nullptr;
            int MaxPickups = 0;
        public:
            void Reset() {
                ObjInterface = nullptr;
                ObjList = nullptr;
                MaxObjects = 0;

                PickupInterface = nullptr;
                PickList = nullptr;
                MaxPickups = 0;
            }

            void Update( )
            {
                while ( !g_Variables.g_Unload )
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

                    if ( !Core::SDK::Pointers::pReplayInterFace )
                        continue;

                    ObjInterface = Core::SDK::Pointers::pReplayInterFace->InterfaceObject( );
                    if ( !ObjInterface ) continue;
                    ObjList = ObjInterface->ObjectList( );
                    if ( !ObjList ) continue;

                    MaxObjects = ObjInterface->MaxObjects( );
                    if (MaxObjects > 0 && MaxObjects < 20000)
                    {
                            std::vector<Core::SDK::Game::ObjectStructure> updatedObjectList;
                            updatedObjectList.reserve(static_cast<size_t>(MaxObjects));

                            CPed* localPed = Core::SDK::Pointers::pLocalPlayer;
                            D3DXVECTOR3 localPos = localPed ? localPed->GetPos() : D3DXVECTOR3(0,0,0);

                            for ( int i = 0; i < MaxObjects; i++ )
                            {
                                uintptr_t CurrentObj = ObjList->Object( i );
                                if ( !CurrentObj )
                                    continue;

                                uintptr_t modelInfo = Mem.Read<uintptr_t>( CurrentObj + 0x20 );
                                if ( !modelInfo || modelInfo <= 0x10000 ) continue;

                                uint32_t hash = Mem.Read<uint32_t>( modelInfo + 0x18 );
                                if ( hash == 0 ) continue;

                                D3DXVECTOR3 objPos = Mem.Read<D3DXVECTOR3>( CurrentObj + 0x90 );
                                float Distance = 0;
                                if (localPed) {
                                    D3DXVECTOR3 diff = objPos - localPos;
                                    Distance = sqrtf( diff.x * diff.x + diff.y * diff.y + diff.z * diff.z );
                                }

                                Core::SDK::Game::ObjectStructure Object;
                                Object.Pointer = CurrentObj;
                                Object.ID = hash;

                                char nameBuf[12];
                                snprintf(nameBuf, sizeof(nameBuf), "0x%08X", hash);
                                Object.Name = nameBuf;

                                Object.Dist = Distance;
                                Object.Pos = objPos;

                                updatedObjectList.push_back( Object );
                            }

                            std::sort( updatedObjectList.begin( ), updatedObjectList.end( ), [ ] ( const auto & lhs, const auto & rhs ) { return lhs.Dist < rhs.Dist; } );

                            {
                                std::lock_guard<std::mutex> lock( Core::SDK::Game::ObjectListMutex );
                                Core::SDK::Game::ObjectList = std::move( updatedObjectList );
                            }
                    }

                    PickupInterface = Core::SDK::Pointers::pReplayInterFace->InterfacePickup( );
                    if ( !PickupInterface ) continue;
                    PickList = PickupInterface->PickupList( );
                    if ( !PickList ) continue;

                    MaxPickups = PickupInterface->MaxPickups( );
                    if (MaxPickups > 0 && MaxPickups < 20000)
                    {
                            std::vector<Core::SDK::Game::PickupStructure> updatedPickupList;
                            updatedPickupList.reserve(static_cast<size_t>(MaxPickups));

                            CPed* localPed = Core::SDK::Pointers::pLocalPlayer;
                            D3DXVECTOR3 localPos = localPed ? localPed->GetPos() : D3DXVECTOR3(0,0,0);

                            for ( int i = 0; i < MaxPickups; i++ )
                            {
                                uintptr_t CurrentPickup = PickList->Pickup( i );
                                if ( !CurrentPickup )
                                    continue;

                                uintptr_t modelInfo = Mem.Read<uintptr_t>( CurrentPickup + 0x20 );
                                if ( !modelInfo || modelInfo <= 0x10000 ) continue;

                                uint32_t hash = Mem.Read<uint32_t>( modelInfo + 0x18 );
                                if ( hash == 0 ) continue;

                                D3DXVECTOR3 pickupPos = Mem.Read<D3DXVECTOR3>( CurrentPickup + 0x90 );
                                float Distance = 0;
                                if (localPed) {
                                    D3DXVECTOR3 diff = pickupPos - localPos;
                                    Distance = sqrtf( diff.x * diff.x + diff.y * diff.y + diff.z * diff.z );
                                }

                                Core::SDK::Game::PickupStructure Pickup;
                                Pickup.Pointer = CurrentPickup;

                                char nameBuf[20];
                                snprintf(nameBuf, sizeof(nameBuf), "Pickup 0x%08X", hash);
                                Pickup.Name = nameBuf;

                                Pickup.Dist = Distance;
                                Pickup.Pos = pickupPos;

                                updatedPickupList.push_back( Pickup );
                            }

                            std::sort( updatedPickupList.begin( ), updatedPickupList.end( ), [ ] ( const auto & lhs, const auto & rhs ) { return lhs.Dist < rhs.Dist; } );

                            {
                                std::lock_guard<std::mutex> lock( Core::SDK::Game::PickupListMutex );
                                Core::SDK::Game::PickupList = std::move( updatedPickupList );
                            }
                    }
                }
            }
        };

        inline cObjectList g_ObjectList;
    }
}