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
        class cVehicleList {
        private:
            CVehInterFace * VehInterface = nullptr;
            CVehicleList * VehList = nullptr;
            int MaxVehicles = 0;
        public:
            void Reset() {
                VehInterface = nullptr;
                VehList = nullptr;
                MaxVehicles = 0;
            }
            void Update( )
            {
                while ( !g_Variables.g_Unload )
                {
                    try {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

                    if (!Core::g_AttachedToGame) continue;
                    if ( !Core::SDK::Pointers::pReplayInterFace )
                        continue;

                    VehInterface = Core::SDK::Pointers::pReplayInterFace->InterfaceVeh( );
                    if ( !VehInterface )
                        continue;

                    VehList = VehInterface->VehicleList( );
                    if ( !VehList )
                        continue;

                    MaxVehicles = VehInterface->MaxVehicles( );
                    if (MaxVehicles <= 0 || MaxVehicles > 10000) continue;

                    std::vector<Core::SDK::Game::VehicleStructure> updatedVehicleList;
                    updatedVehicleList.reserve(static_cast<size_t>(MaxVehicles));

                    CPed* localPed = Core::SDK::Pointers::pLocalPlayer;
                    D3DXVECTOR3 localPos = localPed ? localPed->GetPos() : D3DXVECTOR3(0,0,0);

                    for ( int i = 0; i < MaxVehicles; i++ )
                    {
                        CVehicle * CurrentVeh = VehList->Vehicle( i );
                        if ( !CurrentVeh )
                            continue;

                        uintptr_t vehicleModelInfo = Mem.Read<uintptr_t>( ( uintptr_t ) CurrentVeh + 0x20 );
                        if (!vehicleModelInfo) continue;

                        std::string vehicleName = Mem.ReadString( vehicleModelInfo + 0x298 );
                        if ( vehicleName.empty( ) )
                            continue;

                        D3DXVECTOR3 vehPos = CurrentVeh->GetPos();
                        float Distance = 0;
                        if (localPed) {
                            D3DXVECTOR3 diff = vehPos - localPos;
                            Distance = sqrtf( diff.x * diff.x + diff.y * diff.y + diff.z * diff.z );
                        }

                        Core::SDK::Game::VehicleStructure Vehicle;
                        Vehicle.Pointer = CurrentVeh;
                        Vehicle.Name = Utils::StringToFirstUpperCase( vehicleName );
                        Vehicle.Dist = Distance;
                        Vehicle.IsLocked = CurrentVeh->IsLocked( );
                        Vehicle.Driver = CurrentVeh->GetDriver( );
                        Vehicle.Pos = vehPos;

                        updatedVehicleList.push_back( Vehicle );
                    }

                    std::sort( updatedVehicleList.begin( ), updatedVehicleList.end( ), [ ] ( const auto & lhs, const auto & rhs ) { return lhs.Dist < rhs.Dist; } );

                    {
                        std::lock_guard<std::mutex> lock( Core::SDK::Game::VehicleListMutex );
                        Core::SDK::Game::VehicleList = std::move( updatedVehicleList );
                    }

                    } catch (...) {
                        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                    }
                }
            }
        };

        inline cVehicleList g_VehicleList;
    }
}
