#include "Aimbot.hpp"
#include "HitboneList.hpp"
#include "framework/Globals.hpp"
#include "../../../framework/settings/search.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <chrono>

void Core::Features::cAimbot::SetViewAngles(CPed* Ped, D3DXVECTOR3 BonePos)
{
	if (!Core::SDK::Pointers::pCamGamePlayDirector) return;
	uintptr_t CamFollowPedCamera = Mem.Read<uintptr_t>(Core::SDK::Pointers::pCamGamePlayDirector + g_Offsets.m_CamFollowOffset);
	uintptr_t CamFollowVehicle = CamFollowPedCamera ? Mem.Read<uintptr_t>(CamFollowPedCamera + 0x10) : 0;
    if (!CamFollowPedCamera) return;

    D3DXVECTOR3 CamPos = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + g_Offsets.m_CamPosOffset);
    D3DXVECTOR3 CurrentViewAngles = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + g_Offsets.m_CamViewOffset);
    D3DXVECTOR3 TargetViewAngles = BonePos - CamPos;

    D3DXVec3Normalize(&TargetViewAngles, &TargetViewAngles);

	if (option->param.randomize_angle > 0.0f)
	{
		std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		float j = option->param.randomize_angle * 0.001f;
		TargetViewAngles.x += dist(rng) * j;
		TargetViewAngles.y += dist(rng) * j;
		TargetViewAngles.z += dist(rng) * j;
		D3DXVec3Normalize(&TargetViewAngles, &TargetViewAngles);
	}

	if (Core::SDK::Pointers::pLocalPlayer && Core::SDK::Pointers::pLocalPlayer->InVehicle())
	{
        if (CamFollowVehicle && Mem.Read<float>(CamFollowVehicle + 0x2AC) == -2.f)
		{
			Mem.Write<float>(CamFollowVehicle + 0x2AC, 0.f);
			Mem.Write<float>(CamFollowVehicle + 0x2C0, 111.f);
			Mem.Write<float>(CamFollowVehicle + 0x2C4, 111.f);
		}
	}

	D3DXVECTOR3 Delta = TargetViewAngles - CurrentViewAngles;
	float len = std::sqrt(Delta.x * Delta.x + Delta.y * Delta.y + Delta.z * Delta.z);
    if (len > 1e-6f) {
        float alphaH = (option->param.smooth_horizontal > 0.5f)
            ? (1.0f / (option->param.smooth_horizontal + 1.0f))
            : std::clamp(option->param.aimbot_speed / 100.0f, 0.08f, 0.6f);
        float alphaV = (option->param.smooth_vertical > 0.5f)
            ? (1.0f / (option->param.smooth_vertical + 1.0f))
            : std::clamp(option->param.aimbot_speed / 100.0f, 0.06f, 0.55f);

        alphaH = (std::min)(alphaH * 1.5f, 0.9f);
        alphaV = (std::min)(alphaV * 1.25f, 0.85f);

		if (option->param.aim_curving && option->param.curve_strength > 0.001f) {
			float ease = 1.0f - std::exp(-option->param.curve_strength * len * 4.0f);
			ease = (std::max)(ease, 0.12f);
			alphaH *= ease;
			alphaV *= ease;
		}

		D3DXVECTOR3 FinalAngles = CurrentViewAngles;
		FinalAngles.x = CurrentViewAngles.x + Delta.x * alphaH;
		FinalAngles.y = CurrentViewAngles.y + Delta.y * alphaH;
		FinalAngles.z = CurrentViewAngles.z + Delta.z * alphaV;

		D3DXVECTOR3 ThirdPersonAngles = FinalAngles;
		D3DXVECTOR3 Cam3DAngles = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + g_Offsets.m_CamDataOffset);
		ThirdPersonAngles.z = FinalAngles.z - (CurrentViewAngles.z - Cam3DAngles.z);

		Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + g_Offsets.m_CamViewOffset, FinalAngles);
		Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + g_Offsets.m_CamDataOffset, ThirdPersonAngles);
	}
}

void Core::Features::cAimbot::Start()
{
	while (!g_Variables.g_Unload)
	{
		if (!var->auth.authenticated) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (option->key.enable_aimbot_key && option->key.enable_aimbot_mode == 0 && GetForegroundWindow() == g_Variables.g_hGameWindow) {
			if (Utils::KeyPressedWithDelay(option->key.enable_aimbot_key, 250))
				option->param.enable_aimbot = !option->param.enable_aimbot;
		}

		bool aimActive = option->param.enable_aimbot;
		if (aimActive) {
			if (option->key.enable_aimbot_key != 0) {
				if (option->key.enable_aimbot_mode == 1) {
					aimActive = (GetAsyncKeyState(option->key.enable_aimbot_key) & 0x8000) != 0;
				}
			} else {
				aimActive = false;
			}
		}

		if (aimActive && GetForegroundWindow() != g_Variables.g_hCheatWindow)
		{
			CPed* Ped = Core::SDK::Game::GetClosestPed(option->param.aim_distance, option->param.ignore_npcs, option->param.check_visible);
			if (!Ped) {
				std::this_thread::sleep_for(std::chrono::milliseconds(4));
				continue;
			}

            if (option->param.target_combat_roll)
            {
                if (!Core::SDK::Game::IsPedDoingCombatRoll(Ped)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(4));
                    continue;
                }
            }
            if (option->param.target_jump)
            {
                if (!Core::SDK::Game::IsPedJumping(Ped)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(4));
                    continue;
                }
            }

			D3DXVECTOR3 TargetPos;
			bool hasSkeleton = false;
			do {
				uintptr_t FragInstNMGta = Mem.Read<uintptr_t>((uintptr_t)Ped + g_Offsets.m_FragInst);
				if (!FragInstNMGta) break;
				uintptr_t v9 = Mem.Read<uintptr_t>(FragInstNMGta + 0x68);
				if (!v9) break;
				Core::SDK::Game::cSkeleton_t Skeleton;
				Skeleton.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
				Skeleton.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
				Skeleton.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
				Skeleton.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
				Skeleton.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);
				if (!Skeleton.crSkeletonData.m_BoneIdTable_Slots) break;
				Skeleton.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
				Skeleton.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
				Skeleton.Arg2 = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);
				unsigned int selectedBone = Core::Features::GetHitboneId(option->param.hitbone);
				TargetPos = Core::SDK::Game::GetBonePosComplex(Ped, selectedBone, Skeleton);
				hasSkeleton = true;
			} while (false);
			if (!hasSkeleton)
				TargetPos = Ped->GetBonePosDefault(0);

			float randX = Utils::GenRandomFloat(-0.015f, 0.015f);
			float randY = Utils::GenRandomFloat(-0.015f, 0.015f);
			float randZ = Utils::GenRandomFloat(-0.015f, 0.015f);

			unsigned int hitBoneId = Core::Features::GetHitboneId(option->param.hitbone);
			D3DXVECTOR3 HitOffset;
			if (hitBoneId == SKEL_Head)
				HitOffset = D3DXVECTOR3(randX, randY, 0.08f + randZ);
			else if (hitBoneId == SKEL_Pelvis)
				HitOffset = D3DXVECTOR3(randX, randY, -0.1f + randZ);
			else
				HitOffset = D3DXVECTOR3(randX, randY, randZ);

			D3DXVECTOR2 ScreenPos = Core::SDK::Game::WorldToScreen(TargetPos);

			if (Core::SDK::Game::IsOnScreen(ScreenPos))
			{
				float dx = ScreenPos.x - g_Variables.g_vGameWindowCenter.x;
				float dy = ScreenPos.y - g_Variables.g_vGameWindowCenter.y;
				float FovSq = dx * dx + dy * dy;
				float fovLimit = option->param.fov_size;
				if (fovLimit <= 0.f || FovSq < fovLimit * fovLimit)
				{
					SetViewAngles(Ped, TargetPos + HitOffset);
					std::this_thread::sleep_for(std::chrono::milliseconds(8));
					continue;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(4));
	}
}
