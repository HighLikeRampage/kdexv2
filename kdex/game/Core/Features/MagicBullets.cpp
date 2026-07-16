#include "MagicBullets.hpp"
#include "HitboneList.hpp"
#include "../../../framework/settings/search.h"

uintptr_t Core::Features::cMagicBullets::GetCWeaponObj( )
{
	if (!Core::SDK::Pointers::pLocalPlayer) return 0;
	CWeaponManager* WeaponMgr = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager();
	if (!WeaponMgr) return 0;
	auto CObject = Mem.Read<uintptr_t>( ( uintptr_t ) WeaponMgr + g_Offsets.m_CObject );

	return Mem.Read<uintptr_t>( CObject + g_Offsets.m_CWeapon );
}

void Core::Features::cMagicBullets::Initialize( )
{
	CWeapon = this->GetCWeaponObj( );
	if ( !CWeapon || !g_Offsets.m_MagicBulletsPatch ) return;

	Mem.PatchFunc( g_Offsets.m_MagicBulletsPatch, 4 );
}

void Core::Features::cMagicBullets::Restore( )
{
	std::vector<uint8_t> UpdateMuzzlePosTable
	{
		0x0F, 0x29, 0x4F, 0x20
	};

	Mem.WriteBytes( g_Offsets.m_MagicBulletsPatch, UpdateMuzzlePosTable );
}

void Core::Features::cMagicBullets::Start( )
{
	while ( !g_Variables.g_Unload )
	{
		try {
		if (!var->auth.authenticated) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (option->key.silent_aim_key && option->key.silent_aim_mode == 0 && GetForegroundWindow() == g_Variables.g_hGameWindow) {
			if (Utils::KeyPressedWithDelay(option->key.silent_aim_key, 250))
				option->param.silent_aim = !option->param.silent_aim;
		}

		bool silentActive = option->param.silent_aim;
		if (silentActive) {
			if (option->key.silent_aim_key != 0) {
				if (option->key.silent_aim_mode == 1) {
					silentActive = (GetAsyncKeyState(option->key.silent_aim_key) & 0x8000) != 0;
				}
			} else {
				silentActive = false;
			}
		}

		if ( option->param.magic_bullets && !option->param.silent_check_visible && silentActive && GetForegroundWindow( ) != g_Variables.g_hCheatWindow )
		{

			CPed * Ped = Core::SDK::Game::GetClosestPed( option->param.aim_distance, option->param.silent_ignore_npcs, option->param.silent_check_visible );
			if ( !Ped ) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }

			D3DXVECTOR3 TargetPos;

			uintptr_t FragInstNMGta = Core::Mem.Read<uintptr_t>((uintptr_t)Ped + Core::g_Offsets.m_FragInst);
			if (FragInstNMGta)
			{
				uintptr_t v9 = Core::Mem.Read<uintptr_t>(FragInstNMGta + 0x68);
				if (v9)
				{
					Core::SDK::Game::cSkeleton_t Skeleton;
					Skeleton.m_pSkeleton = Core::Mem.Read<uintptr_t>(v9 + 0x178);
					Skeleton.crSkeletonData.Ptr = Core::Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
					Skeleton.crSkeletonData.m_Used = Core::Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
					Skeleton.crSkeletonData.m_NumBones = Core::Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
					Skeleton.crSkeletonData.m_BoneIdTable_Slots = Core::Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);
					Skeleton.crSkeletonData.m_BoneIdTable = Core::Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
					Skeleton.Arg1 = Core::Mem.Read<D3DXMATRIX>(Core::Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
					Skeleton.Arg2 = Core::Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);

					{
						unsigned int selectedBone = Core::Features::GetHitboneId(option->param.silent_hitbone);
						TargetPos = Core::SDK::Game::GetBonePosComplex(Ped, selectedBone, Skeleton);
					}
				}
			}
			if (TargetPos.x == 0 && TargetPos.y == 0 && TargetPos.z == 0)
				TargetPos = Ped->GetBonePosDefault(0);

			D3DXVECTOR2 HeadToScreen = Core::SDK::Game::WorldToScreen( TargetPos );

			if ( Core::SDK::Game::IsOnScreen( HeadToScreen ) )
			{
				int Fov = std::hypot( HeadToScreen.x - g_Variables.g_vGameWindowCenter.x, HeadToScreen.y - g_Variables.g_vGameWindowCenter.y );
				if ( Fov < option->param.silent_fov_size ) {

					int roll = Utils::GenRandomInt(0, 100);
					bool Miss = roll > option->param.silent_hit_chance;

					D3DXVECTOR3 BulletStartPos;
					if (option->param.silent_aim) {
						static D3DXVECTOR3 lastJitter = { 0, 0, 0 };
						D3DXVECTOR3 currentJitter;
						float range = option->param.silent_jitter;

						int attempts = 0;
						do {
							currentJitter.x = Utils::GenRandomFloat(-range, range);
							currentJitter.y = Utils::GenRandomFloat(-range, range);
							currentJitter.z = Utils::GenRandomFloat(-range, range);
							attempts++;
						} while (attempts < 5 &&
								 std::abs(currentJitter.x - lastJitter.x) < 0.0001f &&
								 std::abs(currentJitter.y - lastJitter.y) < 0.0001f &&
								 std::abs(currentJitter.z - lastJitter.z) < 0.0001f);

						lastJitter = currentJitter;
						BulletStartPos = TargetPos + D3DXVECTOR3(currentJitter.x, currentJitter.y, 0.3f + currentJitter.z);
					} else {
						BulletStartPos = TargetPos;
					}

					auto FinalPos = Miss ? BulletStartPos + D3DXVECTOR3( 0.0, 0.3, 0 ) : BulletStartPos;

					if ( !Initialized ) {
						Initialize( );
						Initialized = true;
					}

					CWeapon = this->GetCWeaponObj();
					if (CWeapon) {
						Mem.Write( CWeapon + 0x20, FinalPos );
					}
				}
				else {
					if ( Initialized ) {
						Restore( );
						Initialized = false;
					}
				}
			}
			else {
				if ( Initialized ) {
					Restore( );
					Initialized = false;
				}
			}
		}
		else
		{
			if ( Initialized ) {
				Restore( );
				Initialized = false;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} catch (...) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}
